// client.cpp - Quiz Game Client (Hostname + Windows Compatible)

#include <iostream>
#include <string>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/socket.h>
#endif

using namespace std;

#define SERVER_HOSTNAME "database.tambytes.cloud"
#define PORT 12345
#define BUFFER_SIZE 2048

int main() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    int wsInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsInit != 0) {
        cerr << "WSAStartup failed with error: " << wsInit << endl;
        return 1;
    }
#endif

    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    string port_str = to_string(PORT);
    int result = getaddrinfo(SERVER_HOSTNAME, port_str.c_str(), &hints, &res);
    if (result != 0 || res == nullptr) {
        cerr << "getaddrinfo failed: " << result << endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int client_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
#ifdef _WIN32
    if (client_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed. Error: " << WSAGetLastError() << endl;
        freeaddrinfo(res);
        WSACleanup();
        return 1;
    }
#else
    if (client_socket < 0) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }
#endif

    if (connect(client_socket, res->ai_addr, static_cast<int>(res->ai_addrlen)) < 0) {
        cerr << "Connection failed.\n";
        freeaddrinfo(res);
#ifdef _WIN32
        closesocket(client_socket);
        WSACleanup();
#else
        close(client_socket);
#endif
        return 1;
    }

    freeaddrinfo(res);
    cout << "Connected to quiz server.\nEnter your player name: ";
    string name;
    getline(cin, name);
    send(client_socket, name.c_str(), static_cast<int>(name.size()), 0);

    char buffer[BUFFER_SIZE]{};
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            cout << "Disconnected from server.\n";
            break;
        }

        buffer[bytes_received] = '\0';
        string received_msg(buffer);
        cout << received_msg;

        // Detect game over and exit
        if (received_msg.find("Game over") != string::npos) {
            cout << "Quiz has ended. Disconnecting...\n";
            break;
        }

        // Prompt for answer if expected
        if (received_msg.length() >= 8 &&
            received_msg.substr(received_msg.length() - 8) == "Answer: ") {
            string answer;
            getline(cin, answer);
            send(client_socket, answer.c_str(), static_cast<int>(answer.size()), 0);
        }
    }

#ifdef _WIN32
    closesocket(client_socket);
    WSACleanup();
#else
    close(client_socket);
#endif

    return 0;
}
