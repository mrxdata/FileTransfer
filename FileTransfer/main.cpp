#include <iostream>
#include <string>
#include <sstream>
#include "client.h"
#include "server.h"

int main() {
    setlocale(LC_ALL, "ru");

    std::string mode;
    std::cout << "Вы хотите запустить сервер или клиент? (server/client): ";
    std::cin >> mode;
    std::cin.ignore();

    if (mode == "server") {
        int port;
        std::string baseDir;

        std::cout << "Введите порт для сервера (по умолчанию 54000): ";
        std::cin >> port;
        std::cin.ignore();
        std::cout << "Введите базовую директорию для сохранения файлов: ";
        std::getline(std::cin, baseDir);

        if (baseDir.empty()) {
            baseDir = std::filesystem::current_path().string();
        }

        startServer(port, baseDir);
    }
    else if (mode == "client") {
        std::string serverAddress;
        std::cout << "Введите адрес сервера в формате IP:PORT: ";
        std::getline(std::cin, serverAddress);

        std::string ip;
        int port;
        size_t colonPos = serverAddress.find(':');
        if (colonPos != std::string::npos) {
            ip = serverAddress.substr(0, colonPos);
            port = std::stoi(serverAddress.substr(colonPos + 1));
        }
        else {
            std::cerr << "Неверный формат IP:PORT" << std::endl;
            return 1;
        }

        runClient(ip, port);
    }
    else {
        std::cerr << "Неверный выбор режима" << std::endl;
    }

    return 0;
}
