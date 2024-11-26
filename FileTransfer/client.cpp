#include <iostream>
#include <string>
#include <fstream>
#include "client.h"
#include "server.h"


std::string extract_ip(const std::string& params) {
	size_t startPos = params.find(" ") + 1;
	size_t colonPos = params.find(":");

	if (colonPos != std::string::npos) {
		return params.substr(startPos, colonPos - startPos);
	}
	return params.substr(startPos);
}

int extract_port(const std::string& params) {
	size_t portPos = 0;
	size_t spacing = 0;
	if (params.find("connect") != std::string::npos) {
		portPos = params.find(":");
		spacing = 1;
	}
	else if (params.find("server") != std::string::npos) {
		portPos = params.find(" ");
		spacing = 1;
	}

	if (portPos != std::string::npos) {
		try {
			int port = std::stoi(params.substr(portPos + spacing));
			if (port >= 0 && port <= 65535) {
				return port;
			}
			else {
				std::cerr << "Error: port is out of range (0-65535)." << std::endl;
				return Server::DEFAULT_PORT;
			}
		}
		catch (const std::invalid_argument&) {
			std::cerr << "Error: port is not a valid number." << std::endl;
		}
		catch (const std::out_of_range&) {
			std::cerr << "Error: port is out of range." << std::endl;
		}
	}
	std::cout << "Warning: no port specified, using default value: " << Server::DEFAULT_PORT << std::endl;
	return Server::DEFAULT_PORT;
}

void Terminal::show() {
	std::cout << std::endl << Terminal::ip_address << ":" << Terminal::port << " - " << Terminal::currentPath.string() << " $ ";
}

void Client::connect_command(std::string ip_address, int port) {
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	
	closesocket(Client::clientSocket);
	Client::clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Client::clientSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation error: " << WSAGetLastError() << std::endl;
		return;
	}
	if (inet_pton(serverAddr.sin_family, ip_address.c_str(), &serverAddr.sin_addr) <= 0) {
		std::cerr << "Wrong IP address " << ip_address.c_str() << std::endl;
		return;
	}
	if (connect(Client::clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Connection error" << std::endl;
		return;
	}

	std::cout	<< "Connection to " 
				<< ip_address 
				<< ":" 
				<< port 
				<< std::endl;

	char buffer[Server::BUFFER_SIZE];
	int bytesReceived = recv(Client::clientSocket, buffer, sizeof(buffer) - 1, 0);

	if (bytesReceived > 0) { 
		buffer[bytesReceived] = '\0';
		std::cout << buffer << std::endl; 
		Terminal::ip_address = ip_address;
		Terminal::port = port;
	}
	else if (bytesReceived == 0) {
		std::cout << "Connection closed by server." << std::endl;
	}
	else {
		std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
	}
}

void Client::send_command(std::string filename) {
	std::ifstream file(Terminal::currentPath / filename, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file " << Terminal::currentPath / filename << std::endl;
		return;
	}

	size_t totalBytes;
	file.seekg(0, std::ios::end);
	std::streamsize fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	if (fileSize == 0) {
		std::cerr << "Error: file is empty." << std::endl;
		return;
	}
	std::cout << "File size: " << fileSize << " bytes" << std::endl;


	char packet[Server::BUFFER_SIZE];
	send(Client::clientSocket, filename.c_str(), filename.length(), 0);
	recv(Client::clientSocket, packet, sizeof(packet), 0);
	send(Client::clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

	while (!file.eof()) {
		file.read(packet, sizeof(packet));
		int bytesRead = static_cast<int>(file.gcount());
		if (send(Client::clientSocket, packet, bytesRead, 0) == SOCKET_ERROR) {
			std::cerr << "Send error" << std::endl;
			break;
		}
		}

	std::cout << "File was sent" << std::endl;
}

void Terminal::ls_command() {
	try {
		for (const auto& entry : fs::directory_iterator(Terminal::currentPath)) {
			std::cout << entry.path().filename().string() << std::endl;
		}
	}
	catch (const fs::filesystem_error& fs_error) {
		std::cerr << "Error: " << fs_error.what() << std::endl;
	}
}

void Terminal::cd_command(std::string path) {
	fs::path newPath = path.empty() ? Terminal::currentPath : fs::path(path);
	if (path == "..") {
		newPath = Terminal::currentPath.parent_path();
	}
	if (!newPath.is_absolute()) {
		newPath = Terminal::currentPath / newPath;
	}
	if (fs::exists(newPath) && fs::is_directory(newPath)) {
		Terminal::currentPath = fs::canonical(newPath);
	}
	else {
		std::cout << "Wrong path: " << newPath.string() << std::endl;
	}
}


void Terminal::commandHandler(std::string command) {
	if (command == "cd") {
		std::cout << "Wrong path";
		return;
	}
	if (command == "connect") {
		std::cout << "Wrong params. Use: connect <IP:PORT> syntax" << std::endl;
		return;
	}
	if (command == "send") {
		std::cout << "Wrong params. Use: send <filename.extension> syntax" << std::endl;
		return;
	}
	if (command.substr(0, command.find(" ")) == "cd") {
		std::string params = command.substr(command.find(" ") + 1); 
		Terminal::cd_command(params);
	}
	if (command == "ls") {
		Terminal::ls_command(); 
	}
	if ((command.substr(0, command.find(" ")) == "connect")) {
		std::string ip_address = extract_ip(command); 
		int port = extract_port(command); 

		if (port != -1) {
			Client::connect_command(ip_address, port);
		}
	}
	if (command.substr(0, command.find(" ")) == "send") {
		Client::send_command(command.substr(command.find("send") + 5));
	}
	if ((command.substr(0, command.find(" ")) == "server")) {

		if (Server::isRunning) {
			std::cerr << "Server is already running" << std::endl;
			return;
		}
		Server::run_server(extract_port(command));
	}
}