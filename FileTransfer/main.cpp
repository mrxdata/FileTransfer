#include <iostream>
#include <string>
#include "client.h"
#include "server.h"

void show() {
    std::cout << R"(
    ##############################################################
    #                                                            #
    #      ________      ________  _________    ________         #
    #     |\   ____\    |\  _____\|\___   ___\ |\   __  \        #
    #     \ \  \___|_   \ \  \__/ \|___ \  \_| \ \  \|\  \       #
    #      \ \_____  \   \ \   __\     \ \  \   \ \   ____\      #
    #       \|____|\  \   \ \  \_|      \ \  \   \ \  \___|      #
    #         ____\_\  \   \ \__\        \ \__\   \ \__\         #
    #        |\_________\   \|__|         \|__|    \|__|         #
    #        \|_________|                                        #
    #                                                            #
    #               * C++ Version: 20                            #
    #               * OpenSSL Version: 3.3.2                     #
    #               * https://github.com/mrxdata                 #
    #                                                            #
    ##############################################################
    )" << std::endl;
}


int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "ru");
    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);  
    if (result != 0) {
        std::cerr << "Winsock initialization error: " << result << std::endl;
        return 1;
    }
    show();
    std::string command;

    while (true) {
        Terminal::show();
        Terminal::commandHandler([](std::string cmd) {
            std::getline(std::cin, cmd); 
            return cmd; 
            }(""));


    }

    WSACleanup();
    return 0;
}
