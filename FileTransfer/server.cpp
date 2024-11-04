#include "server.h"
#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h> 
#include <filesystem>
#include <fstream>

const int BUFFER_SIZE = 1024; 

void receiveFile(SOCKET sock, const std::filesystem::path& baseDir, const char* clientIP, int clientPort);

void startServer(int port, const std::filesystem::path& baseDir) {
    WSADATA wsaData;
    SOCKET server_fd, client_fd;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка инициализации Winsock" << std::endl;
        return;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return;
    }

    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Ошибка прослушивания порта" << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return;
    }

    std::cout << "Сервер запущен на порту " << port << " с базовой директорией \"" << baseDir << "\"" << std::endl;

    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    client_fd = accept(server_fd, (sockaddr*)&clientAddr, &clientAddrSize);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "Ошибка подключения клиента" << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return;
    }

    char clientIP[INET_ADDRSTRLEN]; 
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP)); 
    int clientPort = ntohs(clientAddr.sin_port);
    std::cout << "Клиент " << clientIP << " использует порт " << clientPort << " - Подключен" << std::endl;

    receiveFile(client_fd, baseDir, clientIP, clientPort);

    closesocket(client_fd);
    closesocket(server_fd);
    WSACleanup();
}

void receiveFile(SOCKET sock, const std::filesystem::path& baseDir, const char* clientIP, int clientPort) {
    char fileName[BUFFER_SIZE];
    int bytesReceived = recv(sock, fileName, BUFFER_SIZE, 0);

    if (bytesReceived <= 0) {
        std::cerr << "Ошибка при получении имени файла" << std::endl;
        return;
    }

    std::filesystem::path filePath = baseDir / fileName;
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Ошибка: невозможно создать файл" << std::endl;
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t totalBytesReceived = 0;

    while ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        file.write(buffer, bytesReceived);
        totalBytesReceived += bytesReceived;
    }

    std::cout << "Клиент " << clientIP << " использует порт " << clientPort
        << ", отправил файл \"" << fileName << "\" размером " << totalBytesReceived
        << " байт - Директория: " << baseDir << std::endl;
}
