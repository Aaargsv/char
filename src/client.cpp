#define _WIN32_WINNT 0x0601

#include "chat.h"
#include "utils.h"
#include "custom_recorder.h"
#include "custom_stream.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>


std::string id_name;
std::string you_line;
std::atomic_bool is_sound_off(false);

/// Start sound thread
void sound_receive(SOCKET client_socket)
{
    CustomStream audioStream(client_socket, &is_sound_off);
    audioStream.start();

    // Loop until the sound playback is finished
    while (audioStream.getStatus() != sf::SoundStream::Stopped)
    {
        // Leave some CPU time for other threads
        sf::sleep(sf::milliseconds(100));
    }
}

void sound_send(SOCKET client_socket, std::string &receiver_name)
{
    CustomRecorder recorder(client_socket, id_name, receiver_name);

    // Start capturing audio data
    recorder.start(44100);
    std::cout << "Recording... press enter to stop";
    std::cin.ignore(10000, '\n');
    recorder.stop();
}

/**
 * Send name to the server,
 * receives approval that
 * the name is accepted
 * @param client_socket
 * @return error status
 */

int login_server(SOCKET client_socket)
{
    char buffer[BUFFER_SIZE];

    /// Name entry loop
    while (true) {
        std::cout << "Please enter your ID name (15 characters): ";
        std::cin >> id_name;
        std::cout << std::endl;
        if (id_name.size() > 15) {
            std::cout << "ID is too large" << std::endl;
        } else if ( id_name == "all"      ||
                    id_name == "server"   ||
                    id_name == "[file]"   ||
                    id_name == "sound on" ||
                    id_name == "sound off") {
            std::cout << "Invalid name" << std::endl;
        } else {
            break;
        }
    }

    /// Send name and receive approval loop
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        buffer[0] = ID_PACK;
        strncpy(buffer + 1, id_name.c_str(), ID_NAME_SIZE);
        send(client_socket, buffer, BUFFER_SIZE, 0);
        int error_code = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (error_code == -1) {
            std::cerr << "[Error]: can't receive" << std::endl;
            return 1;
        }
        if (buffer[0] == SERVER_ACCEPT_NAME) {
            break;
        } else if(buffer[0] == SERVER_REJECT_NAME) {
            std::cout << "This ID name is already taken. Enter another ID name: ";
            std::cin >> id_name;
            std::cout << std::endl;
        }
    }
    you_line = id_name + ">";
    return 0;
}

void send_to_server(SOCKET client_socket)
{

    char buffer[BUFFER_SIZE];
    std::string receiver_name;

    /// Input loop
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        std::string input;
        std::cout << you_line;
        std::getline(std::cin, input);

        std::string::size_type beg_name_idx = input.find_first_not_of(" \t");
        if (beg_name_idx == std::string::npos) {
            clear_line();
            continue;
        }

        std::string::size_type end_name_idx = input.find_first_of(" \t", beg_name_idx);
        if (end_name_idx == std::string::npos) {
            clear_line();
            continue;
        }

        std::string::size_type data_beg_idx = input.find_first_not_of(" \t", end_name_idx);
        if (data_beg_idx == std::string::npos) {
            std::cout << "write some message" << std::endl;
            continue;
        }

        /// Get receiver name
        receiver_name = input.substr(beg_name_idx, end_name_idx);
        if (receiver_name == id_name) {
            clear_line();
            continue;
        }

        /// If file transfer
        if (input.compare(data_beg_idx, 6, "[file]") == 0 ) {

            // [type_byte][sender_name][reciver_name][file_name][file_size][data]
            auto file_path_beg_idx = input.find_first_not_of(" \t", data_beg_idx + strlen("[file]"));
            if (file_path_beg_idx == std::string::npos) {
                std::cout << "Please enter the file path" << std::endl;
                continue;
            }

            std::string file_path = input.substr(file_path_beg_idx); // example: C:\project\temp.txt
            std::cout << "file path: " << file_path << std::endl;
            std::ifstream file(file_path);
            if (!file) {
                std::cout << "[Error]: can't open the file" << std::endl;
                continue;
            }
            std::string file_name;
            auto file_name_beg_idx = file_path.find_last_of("/\\");
            if (file_name_beg_idx != std::string::npos) {
                file_name = file_path.substr(file_name_beg_idx + 1); // example: temp.txt
            } else {
                file_name = file_path;
            }

            int len = 0;
            buffer[0] = FILE_PACK; // 0x01 for file transfer;
            len++;
            strncpy(buffer + len, id_name.c_str(), ID_NAME_SIZE); // sender name
            len += ID_NAME_SIZE;
            strncpy(buffer + len, receiver_name.c_str(), ID_NAME_SIZE); // receiver name
            len += ID_NAME_SIZE;
            strncpy(buffer + len, file_name.c_str(), file_name.size());
            len += FILE_NAME_SIZE;
            file.seekg(0, std::ios::end);
            int file_size = file.tellg();
            file.seekg(0, std::ios::beg);

            while (file_size != 0) {
                int transfer_data_size = (file_size <= (BUFFER_SIZE - len - sizeof(int))) ?
                        file_size: BUFFER_SIZE - len - sizeof(int);
                memcpy(buffer + len, &transfer_data_size, sizeof(int));
                file.read(buffer + len + sizeof(int), transfer_data_size);
                int error_code;
                error_code = send(client_socket, buffer, BUFFER_SIZE, 0);
                if (error_code == SOCKET_ERROR) {
                    std::cerr << "[Error]: can't send #";
                    std::cerr << WSAGetLastError() << std::endl;
                    file.close();
                    continue;
                }
                file_size -= transfer_data_size;
            }

            file.close();

            /// If sound transfer
        } else if ((input.compare(data_beg_idx, 8, "sound on") == 0)) {
            if (!sf::SoundBufferRecorder::isAvailable()) {
                std::cerr << "[Error]: microphone isn't available" << std::endl;
                continue;
            } else {
                std::cerr << "microphone is on" << std::endl;
                int len = 0;
                buffer[0] = SOUND_CONNECT; // 0x00 for text transfer;
                len++;
                strncpy(buffer + len, id_name.c_str(), ID_NAME_SIZE); // sender name
                len += ID_NAME_SIZE;
                strncpy(buffer + len, receiver_name.c_str(), ID_NAME_SIZE);
                len += ID_NAME_SIZE;
                send(client_socket, buffer, BUFFER_SIZE, 0);

                sound_send(client_socket, receiver_name);

            }
            /// If you want to disconnect from audio
        } else if ((input.compare(0, 9, "sound off") == 0 )) {
            is_sound_off = true;
            /// If text message transfer
        } else {
            int len = 0;
            buffer[0] = TEXT_PACK; // 0x00 for text transfer;
            len++;
            strncpy(buffer + len, id_name.c_str(), ID_NAME_SIZE); // sender name
            len += ID_NAME_SIZE;
            strncpy(buffer + len, receiver_name.c_str(), ID_NAME_SIZE);
            len += ID_NAME_SIZE;
            strncpy(buffer + len, input.substr(data_beg_idx).data(), BUFFER_SIZE - 1 - 2 * ID_NAME_SIZE);
            send(client_socket, buffer, BUFFER_SIZE, 0);

        }
    }
}

void receive_from_server(SOCKET client_socket)
{
    char buffer[BUFFER_SIZE + 1];
    /// Receive loop
    while (true) {
        memset(buffer, 0, BUFFER_SIZE + 1);
        int error_code;
        std::string sender_name;

        error_code = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (error_code == -1) {
            std::cerr << "[Error]: can't receive" << std::endl;
            return;
        }

        /// [type_byte][sender_name][reciver_name][file_name][file_size][data]
        /// if file receiving
        if (buffer[0] == FILE_PACK) {
            char file_name[33];
            int len = 1;
            sender_name = buffer + 1;
            len += ID_NAME_SIZE;
            len += ID_NAME_SIZE;
            memset(file_name, 0, 33);
            strncpy(file_name, buffer + len, 32);
            len += FILE_NAME_SIZE;
            int file_size;
            memcpy(&file_size, buffer + len, sizeof(file_size));
            len += sizeof(file_size);
            std::ofstream file(file_name, std::ios::binary | std::ios::app);
            if (!file) {
                std::cerr << "[Error]: can't create the file" << std::endl;
                continue;
            }
            file.write(buffer + len, file_size);
            file.close();
            /// if sound receiving
        } else if (buffer[0] == SOUND_CONNECT) {
            sound_receive(client_socket);
            /// if text msg receiving
        } else if (buffer[0] == TEXT_PACK) {
            sender_name = buffer + 1;
            clear_line();
            std::cout << sender_name << "> " << buffer + 1 + 2 * ID_NAME_SIZE << std::endl;
            std::cout << you_line;
            /*
             *  /\_/\
               ( o.o )
                > ^ <
             */
        }
    }
}


int main(int argc, char *argv[])
{
    char server_IP[16];
    int port;

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

    if (login_server(client_socket)) {
        std::cerr << "[Error]: can't login # " ;
        std::cerr << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    } else {
        std::cout << "You have successfully logged in" << std::endl ;
    }

    /**
     * @brief
     * Sending and receiving
     * work in parallel in
     * two threads
     */

    std::thread thread_send(send_to_server, client_socket);
    std::thread thread_receive(receive_from_server, client_socket);
    thread_send.join();
    thread_receive.join();
    return 0;
}