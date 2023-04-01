#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <cstring>
#include <set>

#define BUFFER_SIZE 4096

std::mutex mtx;
std::set<SOCKET> set_clients_sockets;

void handle_client(SOCKET client_socket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    mtx.lock();
    set_clients_sockets.insert(client_socket);
    mtx.unlock();

    while (true) {
        int error_code = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (error_code == 0 || error_code == SOCKET_ERROR)
            break;
        for (auto it = set_clients_sockets.begin(); it != set_clients_sockets.end(); ++it) {
            if (*it != client_socket) {
                send(*it, buffer, BUFFER_SIZE, 0);
            }
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    mtx.lock();
    set_clients_sockets.erase(client_socket);
    mtx.unlock();
    std::cout << "Client disconnected" << std::endl;
    closesocket(client_socket);
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
