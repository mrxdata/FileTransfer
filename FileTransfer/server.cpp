#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "server.h"
#include "client.h"
#include "certmanager.h"

#pragma comment(lib, "ws2_32.lib")

void handleClient(SOCKET clientSocket, sockaddr_in clientAddr, SSL_CTX* ctx) {
	SSL* ssl = SSL_new(ctx);
	SSL_set_fd(ssl, clientSocket);

	if (SSL_accept(ssl) <= 0) {
		std::cerr << "SSL handshake failed\n";
		SSL_free(ssl); 
		return;
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
	SSL_CTX* ctx = CertManager::setupSSLContext(true);
	if (!ctx) {
		Server::isRunning = false;
		return;
	}

	Server::serverAddr.sin_family = AF_INET;
	Server::serverAddr.sin_port = htons(port);
	Server::serverAddr.sin_addr.s_addr = INADDR_ANY;

	std::cout	<< "Server is running on: " 
				<< inet_ntoa(serverAddr.sin_addr) 
				<< ":" 
				<< ntohs(serverAddr.sin_port) 
				<< std::endl;

	SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listeningSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation error: " << WSAGetLastError() << std::endl;
		Server::isRunning = false;
		return;
	}
	if (bind(listeningSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Socket binding error: " << WSAGetLastError() << std::endl;
		Server::isRunning = false;
		return;
	}

	if (listen(listeningSocket, MAX_CONNECTIONS) == SOCKET_ERROR) {
		std::cerr << "Socket listening error: " << WSAGetLastError() << std::endl;
		Server::isRunning = false;
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