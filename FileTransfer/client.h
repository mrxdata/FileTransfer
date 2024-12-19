#pragma once

#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <filesystem>
#include <fstream>
#include <openssl/ssl.h>

#pragma comment(lib, "ws2_32.lib")

namespace fs = std::filesystem;

class Terminal {
public:
	static inline fs::path currentPath = fs::current_path();
	static inline std::string ip_address = "127.0.0.1";
	static inline int port;
	static void ls_command();
	static void cd_command(std::string path);
	static void commandHandler(std::string command);
	static void show();
};

class Client {
public:
	static inline SSL* ssl;
	static inline SSL_CTX* ctx;
	static inline SOCKET clientSocket;
	static void connect_command(std::string ip_address, int port);
	static void send_command(std::string filename);
};