#include <iostream>
#include <filesystem>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include "client.h"

#pragma comment(lib, "ws2_32.lib")
const int BUFFER_SIZE = 1024;

void sendFile(SOCKET sock, const std::string& filePath);

void runClient(const std::string& serverIP, int serverPort) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка инициализации Winsock" << std::endl;
        return;
    }

    SOCKET client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    if (connect(client_fd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Ошибка подключения к серверу" << std::endl;
        closesocket(client_fd);
        WSACleanup();
        return;
    }

    std::cout << "Вы успешно подключены к серверу!" << std::endl;
    std::string command, currentDir = std::filesystem::current_path().string();

    while (true) {
        std::cout << serverIP << ":" << serverPort << " - " << currentDir << " $ ";
        std::getline(std::cin, command);

        if (command.starts_with("cd ")) {
            std::string path = command.substr(3);
            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                currentDir = path;
            }
            else {
                std::cerr << "Директория не найдена" << std::endl;
            }
        }
        else if (command.starts_with("send ")) {
            std::string filePath = currentDir + "\\" + command.substr(5);
            std::ifstream file(filePath, std::ios::binary);

            if (!file) {
                std::cerr << "Не удалось открыть файл для отправки" << std::endl;
                continue;
            }

            sendFile(client_fd, filePath);
        }
        else if (command == "exit") {
            break;
        }
        else {
            std::cerr << "Неизвестная команда" << std::endl;
        }
    }

    closesocket(client_fd);
    WSACleanup();
}

void sendFile(SOCKET sock, const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Ошибка: файл не найден" << std::endl;
        return;
    }

    std::filesystem::path p(filePath);
    std::string fileName = p.filename().string();
    size_t fileSize = std::filesystem::file_size(filePath);

    send(sock, fileName.c_str(), fileName.size() + 1, 0);

    char buffer[BUFFER_SIZE];
    size_t totalBytesSent = 0;

    while (file.read(buffer, sizeof(buffer))) {
        int bytesToSend = static_cast<int>(file.gcount());
        send(sock, buffer, bytesToSend, 0);
        totalBytesSent += bytesToSend;
    }

    if (file.gcount() > 0) {
        send(sock, buffer, static_cast<int>(file.gcount()), 0);
        totalBytesSent += file.gcount();
    }

    std::cout << "Файл " << fileName << " (" << totalBytesSent << " байт) отправлен!" << std::endl;
}
