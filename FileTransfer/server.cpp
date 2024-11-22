#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <thread>
#include <fstream>
#include <filesystem>
#include "server.h"
#include "client.h"
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

void handleClient(SOCKET clientSocket, sockaddr_in clientAddr) {
	char packet[Server::BUFFER_SIZE];
	char clientIP[INET_ADDRSTRLEN];
	int bytesReceived;
	int clientPort = ntohs(clientAddr.sin_port);
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));

	std::cout << "New connection - IP: " << clientIP << ":" << clientPort << " - Accept? (y/n): ";
	char response;
	std::cin >> response;

	if (response == 'y' || response == 'Y') {
		std::string message = "Connection established";

		if (int sendResult = send(clientSocket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
			std::cerr << "Failed to send message: " << WSAGetLastError() << std::endl;
		}
		std::cout << message << std::endl;

	}
	else {
		std::string message = "Connection lost";

		if (int sendResult = send(clientSocket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
			std::cerr << "Failed to send message: " << WSAGetLastError() << std::endl;
		}
		std::cout << message << std::endl;
	}

	

	while (Server::isRunning) 
	{
		bytesReceived = recv(clientSocket, packet, sizeof(packet), 0);
		std::string fileName(packet, bytesReceived);
		const char* status = "OK";
		std::cout << "Status: OK" << std::endl;
		send(clientSocket, status, sizeof(status), 0);
		std::ofstream outFile(Terminal::currentPath / fileName, std::ios::binary);
		std::cout << "Status: waiting" << std::endl;
		int i = 0;
		//не понимает что файл получен
		while ((bytesReceived = recv(clientSocket, packet, sizeof(packet), 0)) > 0) {
			outFile.write(packet, bytesReceived);
			std::cout << "Status: Receiving... " << i << std::endl;
			i++;
		}
		outFile.close();
		std::cout << "File was received successfully" << std::endl;
	}
	closesocket(clientSocket);
}

void Server::run_server(int port) {
	Server::isRunning = true;
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

		std::thread clientThread(handleClient, clientSocket, clientAddr);
		clientThread.detach();
	}
	Server::isRunning = false;
}