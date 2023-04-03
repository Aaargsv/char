#define _WIN32_WINNT 0x0601

#include "chat.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <fstream>
#include <cstring>
#include <unordered_map>

std::mutex mtx;
std::unordered_map<std::string , SOCKET> map_clients_sockets;
std::string  server_name = "server";

void handle_client(SOCKET client_socket)
{
    char buffer[BUFFER_SIZE + 1];
    memset(buffer, 0, BUFFER_SIZE + 1);
    std:: string client_id_name;

    // confirm client id

    while (true) {
        int error_code = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (error_code == 0 || error_code == SOCKET_ERROR) {
            std::cout << "Client disconnected" << std::endl;
            closesocket(client_socket);
            return;
        }
        if (buffer[0] != ID_PACK)
            continue;
        client_id_name = buffer + 1;
        std::cout << client_id_name << std::endl;
        mtx.lock();
        auto search_name_it = map_clients_sockets.find(client_id_name);
        if (search_name_it == map_clients_sockets.end()) {
            map_clients_sockets[client_id_name] = client_socket;
            buffer[0] = SERVER_ACCEPT_NAME;
            strncpy(buffer + 1, server_name.c_str(), server_name.size());
            send(client_socket, buffer, BUFFER_SIZE, 0);
            mtx.unlock();
            break;
        } else {
            buffer[0] = SERVER_REJECT_NAME;
            strncpy(buffer + 1, server_name.c_str(), server_name.size());
            send(client_socket, buffer, BUFFER_SIZE, 0);
        }
        mtx.unlock();
    }

    // transfer
    while (true) {
        int error_code = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (error_code == 0 || error_code == SOCKET_ERROR)
            break;

        std::string  sender_name = buffer + 1;
        std::string receiver_name = buffer + 1 + ID_NAME_SIZE;

        if (receiver_name == "all") {
            for (auto it = map_clients_sockets.begin(); it != map_clients_sockets.end(); ++it) {
                if (it->first != sender_name) {
                    send(it->second, buffer, BUFFER_SIZE, 0);
                }
            }
        } else {
            auto it = map_clients_sockets.find(receiver_name);
            if (it != map_clients_sockets.end()) {
                send(it->second, buffer, BUFFER_SIZE, 0);
            }
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    mtx.lock();
    map_clients_sockets.erase(client_id_name);
    mtx.unlock();
    std::cout << "Client disconnected" << std::endl;
    closesocket(client_socket);
}

int main(int argc, char *argv[])
{
    char server_IP[16];
    int port;

    //const char *server_IP = "192.168.0.101"; //127.0.0.1
    //const int port = 1024;

    if (argc >= 3) {
        strncpy(server_IP, argv[1], 16);
        port = std::stoi(std::string(argv[2]));
    } else if (argc <= 2) {
        std::string settings_file_name;
        if (argc == 2) {
            settings_file_name = argv[1];
        } else if (argc == 1) {
            settings_file_name = "settings.txt";
        }

        std::ifstream settings_file(settings_file_name);
        if (!settings_file) {
            std::cerr << "[Error]: can't open settings file" << std::endl;
            return 1;
        }
        std::string buffer_ip;
        std::string buffer_port;
        if (!(settings_file >> buffer_ip >> buffer_port)) {
            std::cerr << "[Error]: wrong settings format" << std::endl;
            return 1;
        }
        strncpy(server_IP, buffer_ip.c_str(), 16);
        port = std::stoi(buffer_port);
    }


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

    // Server socket initialization
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "[Error]: can't initialize socket # ";
        std::cerr << WSAGetLastError() << std:: endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    else
        std::cout << "Server socket initialization is OK" << std::endl;

    // Server socket binding
    sockaddr_in server_info;
    memset(&server_info, 0, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr = ip_to_num;
    server_info.sin_port = htons(port);

    error_code = bind(server_socket, (sockaddr*)&server_info, sizeof(server_info));
    if ( error_code != 0 ) {
        std::cerr << "[Error]: can't bind socket to server info. Error # ";
        std::cerr <<  WSAGetLastError() << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    else
        std::cout << "Binding socket to Server info is OK" << std::endl;

    //Starting to listen to any Clients
    error_code = listen(server_socket, SOMAXCONN);
    if ( error_code != 0 ) {
        std::cerr << "[Error]: can't start to listen to. Error # ";
        std::cerr <<  WSAGetLastError() << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    else {
        std::cout << "Listening..." << std::endl;
    }

    sockaddr_in client_info;
    int client_info_size = sizeof(client_info);
    while (true) {
        //Client socket creation and acception in case of connection

        memset(&client_info, 0, sizeof(client_info));
        SOCKET client_socket = accept(server_socket, (sockaddr*)&client_info, &client_info_size);

        if (client_socket == INVALID_SOCKET) {
            std::cerr << "[Error]: can't accept a client. Error # ";
            std::cerr <<  WSAGetLastError() << std::endl;
            closesocket(server_socket);
            closesocket(client_socket);
            WSACleanup();
            return 1;
        }
        else {
            std::cout << "Connection to a client established successfully" << std::endl;
            char client_IP[32];
            inet_ntop(AF_INET, &client_info.sin_addr, client_IP, INET_ADDRSTRLEN);	// Convert connected client's IP to standard string format
            std::cout << "Client connected with IP address " << client_IP << std::endl;

        }

        std::thread thread_handle_client(handle_client, client_socket);
        thread_handle_client.detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
