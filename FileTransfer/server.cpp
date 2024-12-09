#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <thread>
#include <fstream>
#include <filesystem>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "server.h"
#include "client.h"

#pragma comment(lib, "ws2_32.lib")

SSL_CTX* setupServerContext(const char* certFile, const char* keyFile, const char* caFile) {
	
	SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
	if (!ctx) {
		std::cerr << "Failed to create SSL context" << std::endl;
		return nullptr;
	}

	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);

	if (SSL_CTX_use_certificate_file(ctx, certFile, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		return nullptr;
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM) <= 0) {
		ERR_print_errors_fp(stderr);
		return nullptr;
	}

	if (SSL_CTX_load_verify_locations(ctx, caFile, nullptr) <= 0) {
		ERR_print_errors_fp(stderr);
		return nullptr;
	}

	return ctx;
}

void handleClient(SOCKET clientSocket, sockaddr_in clientAddr, SSL_CTX* ctx) {
	SSL* ssl = SSL_new(ctx);
	SSL_set_fd(ssl, clientSocket);

	if (SSL_accept(ssl) <= 0) {
		std::cerr << "SSL handshake failed\n";
		ERR_print_errors_fp(stderr); 
		SSL_free(ssl); 
		return;
	}

	X509* cert = SSL_get_peer_certificate(ssl);
	if (!cert) {
		std::cerr << "No client certificate provided\n";
	}
	else {
		long verifyResult = SSL_get_verify_result(ssl);
		if (verifyResult != X509_V_OK) {
			std::cerr << "Client certificate verification failed: " << verifyResult << "\n";
		}
		else {
			std::cout << "Client certificate verified successfully\n";
		}
		X509_free(cert);
	}

	char packet[Server::BUFFER_SIZE]{};
	char clientIP[INET_ADDRSTRLEN]{};
	int bytesReceived = 0;
	int clientPort = ntohs(clientAddr.sin_port);
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));

	std::cout << "New connection - IP: " << clientIP << ":" << clientPort << " - Accept? (y/n): ";
	char response;
	std::cin >> response;

	if (response == 'y' || response == 'Y') {
		std::string message = "Connection established";
		SSL_write(ssl, message.c_str(), message.size());
		std::cout << message << std::endl;

	}
	else {
		std::string message = "Connection lost";
		SSL_write(ssl, message.c_str(), message.size());
		std::cout << message << std::endl;
	}

	while (Server::isRunning) 
	{
		std::streamsize fileSize;
		std::streamsize bytesReceivedTotal = 0;

		bytesReceived = SSL_read(ssl, packet, sizeof(packet) - 1);;
		if (bytesReceived <= 0) continue;
		std::string fileName(packet, bytesReceived);

		const char* status = "OK";
		SSL_write(ssl, status, strlen(status));

		std::ofstream outFile(Terminal::currentPath / fileName, std::ios::binary);

		SSL_read(ssl, reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

		while (bytesReceivedTotal < fileSize) {
			bytesReceived = SSL_read(ssl, packet, sizeof(packet));
			outFile.write(packet, bytesReceived);
			bytesReceivedTotal += bytesReceived;
		}	
		outFile.close();

		std::cout << "File was received" << std::endl;
	}
	closesocket(clientSocket);
	SSL_shutdown(ssl);
	SSL_free(ssl);
}

void Server::run_server(int port) {
	Server::isRunning = true;
	std::string certFile = (fs::current_path() / "cert/server/server.crt").string();
	std::string keyFile = (fs::current_path() / "cert/server/server.key").string();
	std::string caFile = (fs::current_path() / "cert/CA/ca.crt").string();
	SSL_CTX* ctx = setupServerContext(certFile.c_str(), keyFile.c_str(), caFile.c_str());
	if (!ctx) return;

	Server::serverAddr.sin_family = AF_INET;
	Server::serverAddr.sin_port = htons(port); //22
	inet_pton(serverAddr.sin_family, "127.0.0.1", &serverAddr.sin_addr); 

	std::cout	<< "Server is running on: " 
				<< inet_ntoa(serverAddr.sin_addr) 
				<< ":" 
				<< ntohs(serverAddr.sin_port) 
				<< std::endl;

	SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listeningSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation error: " << WSAGetLastError() << std::endl;
		return;
	}
	if (bind(listeningSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Socket binding error: " << WSAGetLastError() << std::endl;
		return;
	}

	if (listen(listeningSocket, MAX_CONNECTIONS) == SOCKET_ERROR) {
		std::cerr << "Socket listening error: " << WSAGetLastError() << std::endl;
		return; 
	}
	std::cout << "Listening..." << std::endl;

	while (isRunning) {
		sockaddr_in clientAddr;
		int clientAddrSize = sizeof(clientAddr);

		SOCKET clientSocket = accept(listeningSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
			continue;
		}

		std::thread clientThread(handleClient, clientSocket, clientAddr, ctx);
		clientThread.detach();
	}
	Server::isRunning = false;
	SSL_CTX_free(ctx);
	closesocket(listeningSocket);
}