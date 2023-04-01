#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

#define BUFFER_SIZE 4096

void send_to_server(SOCKET client_socket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        std::string::size_type idx = input.find_first_not_of(" \t");
        if (idx == std::string::npos)
            continue;
        if (input.compare(0, 7, "[FILE]:") == 0 ) {
            idx = input.find_first_not_of(" \t");
            if (idx == std::string::npos) {
                std::cout << "Please enter the file path" << std::endl;
                continue;
            }

            std::string file_path = input.substr(7); // example: C:\project\temp.txt
            std::ifstream file(file_path);
            if (!file) {
                std::cout << "[Error]: can't open the file" << std::endl;
                continue;
            }
            std::string file_name;
            idx = file_path.find_last_of("/\\");
            if (idx != std::string::npos) {
                file_name = file_path.substr(idx + 1); // example: temp.txt
            } else {
                file_name = file_path;
            }

            int len = 0;
            buffer[0] = 0x01; // 0x01 for file transfer;
            len++;
            strncpy(buffer + len, file_name.c_str(), 32);
            len += 32;
            std::stringstream ss;
            ss << file.rdbuf();
            strncpy(buffer + len, ss.str().c_str(), BUFFER_SIZE - len);
        }
        else
            std::cin.getline(buffer, BUFFER_SIZE);

        send(client_socket, buffer, BUFFER_SIZE, 0);
    }
}

void receive_from_server(SOCKET client_socket)
{
    char buffer[BUFFER_SIZE + 1];
    memset(buffer, 0, BUFFER_SIZE + 1);
    while (true) {
        int error_code;
        error_code = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (error_code == -1) {
            std::cout << "[Error]: can't receive" << std::endl;
            return;
        }

        std::cout << buffer << std::endl;
    }
}

int main()
{
    const char *server_IP = "127.0.0.1";
    const int port = 1024;
    int error_code;
    in_addr ip_to_num;

    error_code = inet_pton(AF_INET, server_IP, &ip_to_num);
    if (error_code <= 0) {
        std::cerr << "[Error]: can't translate IP address to special numeric format" << std::endl;
        return 1;
    }

    // WinSock initialization
    WSADATA wsData;
    error_code = WSAStartup(MAKEWORD(2,2), &wsData);
    if ( error_code != 0 ) {
        std::cerr << "[Error]: WinSock version initialization # ";
        std::cerr << WSAGetLastError() << std:: endl;
        return 1;
    }
    else
        std::cout << "WinSock initialization is OK" << std::endl;

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "[Error]: can't initialize socket # ";
        std::cerr << WSAGetLastError() << std:: endl;
        closesocket(client_socket);
        WSACleanup();
    }
    else
        std::cout << "Client socket initialization is OK" << std::endl;

    sockaddr_in server_info;
    memset(&server_info, 0, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr = ip_to_num;
    server_info.sin_port = htons(port);

    error_code = connect(client_socket, (sockaddr*)&server_info, sizeof(server_info));

    if (error_code != 0) {
        std::cerr << "[Error]: can't connect to server # " ;
        std::cerr << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    else
        std::cout << "Connection established successfully" << std::endl;

    std::thread thread_send(send_to_server, client_socket);
    std::thread thread_receive(receive_from_server, client_socket);
    thread_send.join();
    thread_receive.join();
    return 0;
}