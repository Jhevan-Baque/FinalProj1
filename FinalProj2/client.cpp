// client.cpp - Quiz Game Client (Cross Platform))

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

#define SERVER_IP "147.189.169.152"
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

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    int client_socket =
#ifdef _WIN32
        socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed. Error: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }
#else
        socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return 1;
    }
#endif

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Connection failed.\n";
#ifdef _WIN32
        closesocket(client_socket);
        WSACleanup();
#else
        close(client_socket);
#endif
        return 1;
    }

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
