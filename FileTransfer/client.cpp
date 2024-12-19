#include "client.h"
#include "server.h"
#include "certmanager.h"

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
	Client::ctx = CertManager::setupSSLContext(false);
	if (!Client::ctx) return;

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

	std::cout << "Connection to "
		<< ip_address
		<< ":"
		<< port
		<< std::endl;

	Client::ssl = SSL_new(Client::ctx);
	SSL_set_fd(ssl, clientSocket);

	if (SSL_connect(ssl) <= 0) {
		std::cerr << "SSL handshake failed" << std::endl;
		SSL_free(Client::ssl);
		closesocket(Client::clientSocket);
		SSL_CTX_free(Client::ctx);
		return;
	}

	std::cout << "Waiting for handshake" << std::endl;

	char buffer[Server::BUFFER_SIZE]{};
	int bytesReceived = SSL_read(ssl, buffer, sizeof(buffer) - 1);

	if (bytesReceived > 0) {
		buffer[bytesReceived] = '\0';
		std::cout << "Server response: " << std::string(buffer, bytesReceived) << std::endl;
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
	if (Client::ssl == nullptr) {
		std::cerr << "Error: Not connected to the server." << std::endl;
		return;
	}

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
	SSL_write(Client::ssl, filename.c_str(), filename.length());
	SSL_read(Client::ssl, packet, sizeof(packet));
	SSL_write(Client::ssl, reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

	while (!file.eof()) {
		file.read(packet, sizeof(packet));
		int bytesRead = static_cast<int>(file.gcount());
		if (SSL_write(Client::ssl, packet, bytesRead) == SOCKET_ERROR) {
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

void handleCrtKeyCommand(const std::string& command) {
	std::vector<std::string> args;
	std::string temp;
	std::istringstream stream(command);

	while (stream >> temp) {
		args.push_back(temp);
	}

	if (args.empty() || args.at(0) != "crtkey") {
		std::cerr << "Usage: crtkey <gen|save|use> [filename]\n";
		return;
	}

	if (args.at(1) == "gen") {
		std::cout << "Generating key...\n";
		CertManager::generatePEM();

		if (args.size() == 4 && args.at(2) == "save") {
			std::string filename = args.at(3);
			std::cout << "Saving key to file: " << filename << "\n";
			CertManager::savePEM(filename);
		}
	}
	else if (args.at(1) == "save") {
		if (args.size() < 3) {
			std::cerr << "Usage: crtkey save <filename>\n";
			return;
		}

		std::string filename = args.at(2);
		std::cout << "Saving key to file: " << filename << "\n";
		CertManager::savePEM(filename);
	}
	else if (args.at(1) == "use") {
		if (args.size() < 3) {
			std::cerr << "Usage: crtkey use <filename>\n";
			return;
		}
		std::cout << "Using PEM file: " << args.at(2) << "\n";
		CertManager::usePEM(args.at(2));
	}
	else {
		std::cerr << "Invalid command or arguments. Usage: crtkey <gen|save|use> [filename]\n";
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

	if (command.substr(0, command.find(" ")) == "crtkey") {
		handleCrtKeyCommand(command);
	}
}