#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

namespace fs = std::filesystem;

class Server {
public:
	static constexpr unsigned int BUFFER_SIZE = 4096;
	static constexpr unsigned int DEFAULT_PORT = 22;
	static constexpr unsigned int MAX_CONNECTIONS = 10;
	static inline bool isRunning;
	static inline sockaddr_in serverAddr;
	static void run_server(int port);
};
