#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

using namespace std;

const int PORT = 12345;
const int BUFFER_SIZE = 1024;
const int MIN_PLAYERS = 2;
const int START_DELAY_SECONDS = 30;
const int MAX_CLIENTS = 50;

struct ClientInfo {
    SOCKET socket;
    string name;
    int score = 0;
    bool disconnected = false;
};

struct Question {
    string question;
    string answer;
};

mutex clients_mutex;
vector<ClientInfo> clients;
atomic<bool> game_started{false};

// Example questions
vector<Question> questions_pool = {
    {"What is the capital of the Philippines?", "Manila"},
    {"Who was the first president of the Philippines?", "Emilio Aguinaldo"},
    {"What is the national language of the Philippines?", "Filipino"},
    {"What is the largest island in the Philippines?", "Luzon"},
    {"Who is known as the Hero of Tirad Pass?", "Gregorio del Pilar"},
    {"Which Filipino boxer is an 8-division world champion?", "Manny Pacquiao"},
    {"What is the longest river in the Philippines?", "Cagayan River"},
    {"What is the Philippine national fruit?", "Mango"},
    {"Which volcano had a major eruption in 1991?", "Mount Pinatubo"},
    {"What is the name of the Philippine national anthem?", "Lupang Hinirang"},
    {"Who wrote Noli Me Tangere?", "Jose Rizal"},
    {"Which city is known as the Summer Capital of the Philippines?", "Baguio"},
    {"What Filipino festival is held in Cebu every January?", "Sinulog"},
    {"Which island is famous for its white sand beach?", "Boracay"},
    {"Who was the first Filipina Miss Universe?", "Gloria Diaz"},
    {"What is the national bird of the Philippines?", "Philippine Eagle"},
    {"Which body of water lies to the west of the Philippines?", "South China Sea"},
    {"Who is considered the Father of the Philippine Revolution?", "Andres Bonifacio"},
    {"What is the traditional Filipino house called?", "Bahay Kubo"},
    {"Which city is known as the Queen City of the South?", "Cebu City"},
    {"What is the currency of the Philippines?", "Peso"},
    {"What is the Filipino martial art that uses sticks?", "Arnis"},
    {"Which is the highest mountain in the Philippines?", "Mount Apo"},
    {"Which Philippine hero was known as The Sublime Paralytic?", "Apolinario Mabini"},
    {"Which Philippine festival celebrates the bountiful harvest in Lucban, Quezon?", "Pahiyas Festival"},
    {"Which dish is made of pork or chicken stewed in soy sauce and vinegar?", "Adobo"},
    {"Which Philippine island is known for the Chocolate Hills?", "Bohol"},
    {"Who discovered the Philippines for Spain in 1521?", "Ferdinand Magellan"},
    {"Which sea creature is featured on the old 1-centavo coin?", "Bangus"},
    {"What is the Filipino term for traditional tattooing?", "Batok"},
    {"What is the smallest province in the Philippines?", "Batanes"},
    {"What city is known as the Walled City?", "Intramuros"},
    {"Who is the national hero of the Philippines?", "Jose Rizal"},
    {"Which mountain is the highest in the Philippines?", "Mount Apo"},
    {"What is the currency of the Philippines?", "Peso"},
    {"Which city is known as the City of Pines?", "Baguio"},
    {"What Filipino dish is known for its sour broth?", "Sinigang"},
    {"Which island is famous for the Chocolate Hills?", "Bohol"},
    {"Who was the first female president of the Philippines?", "Corazon Aquino"},
    {"What sea lies east of the Philippines?", "Philippine Sea"},
    {"What is the most active volcano in the Philippines?", "Mayon Volcano"},
    {"Which city is known as the Queen City of the South?", "Cebu"},
    {"What is the national tree of the Philippines?", "Narra"},
    {"Which hero is known as the Great Plebeian?", "Andres Bonifacio"},
    {"What is the national flower of the Philippines?", "Sampaguita"},
    {"Which island is known for its surfing waves?", "Siargao"},
    {"What is the Philippine national dance?", "Tinikling"},
    {"What is the main ingredient of Laing?", "Gabi Leaves"},
    {"Which festival is celebrated in Iloilo?", "Dinagyang"},
    {"Who is the Father of the Philippine Constitution?", "Felipe Calderon"},
    {"Which province is known for the Moriones Festival?", "Marinduque"},
    {"Who painted the Spoliarium?", "Juan Luna"},
    {"Which body of water separates Luzon and Mindoro?", "Verde Island Passage"},
    {"What is the name of the Filipino sun god?", "Apolaki"},
    {"What instrument is used in traditional Filipino kulintang music?", "Gongs"},
    {"What fruit is known as the King of Fruits in the Philippines?", "Durian"},
    {"What is the meaning of the word Luzon?", "Big land"},
    {"Which Filipino invented the Moon Buggy used on the moon?", "Eduardo San Juan"},
    {"What is the main mode of transportation in the Philippines?", "Jeepney"}
};

void send_to(SOCKET sock, const string& msg) {
    send(sock, msg.c_str(), static_cast<int>(msg.size()), 0);
}

void broadcast(const string& msg) {
    lock_guard<mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if (!client.disconnected) {
            send_to(client.socket, msg);
        }
    }
}

void close_socket(SOCKET sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE]{};
    int len = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (len <= 0) {
        close_socket(client_socket);
        return;
    }
    buffer[len] = '\0';
    string name(buffer);

    {
        lock_guard<mutex> lock(clients_mutex);
        clients.push_back({client_socket, name});
        cout << name << " joined (" << clients.size() << " players).\n";
        send_to(client_socket, "Welcome " + name + "! Waiting for others...\n");
    }

    if (clients.size() == MIN_PLAYERS && !game_started.exchange(true)) {
        thread([] {
            for (int i = START_DELAY_SECONDS; i > 0; --i) {
                broadcast("Game starts in " + to_string(i) + " seconds...\n");
                this_thread::sleep_for(chrono::seconds(1));
            }
            broadcast("Game is starting now!\n");
        }).detach();
    }
}

void run_quiz_round() {
    shuffle(questions_pool.begin(), questions_pool.end(), mt19937(random_device{}()));
    int num_q = min(5, (int)questions_pool.size());

    for (int i = 0; i < num_q; ++i) {
        string q = "Q" + to_string(i + 1) + ": " + questions_pool[i].question + "\nAnswer: ";
        broadcast(q);

        vector<thread> answer_threads;

        {
            lock_guard<mutex> lock(clients_mutex);
            for (auto& client : clients) {
                answer_threads.emplace_back([&client, i]() {
                    char buffer[BUFFER_SIZE]{};
                    int len = recv(client.socket, buffer, BUFFER_SIZE - 1, 0);
                    if (len <= 0) {
                        client.disconnected = true;
                        close_socket(client.socket);
                        return;
                    }
                    buffer[len] = '\0';
                    string ans(buffer);

                    string correct = questions_pool[i].answer;
                    transform(ans.begin(), ans.end(), ans.begin(), ::tolower);
                    transform(correct.begin(), correct.end(), correct.begin(), ::tolower);

                    if (ans == correct || ans.find(correct) != string::npos) {
                        client.score += 10;
                        send_to(client.socket, "Correct! (+10 pts)\n");
                    } else {
                        send_to(client.socket, "Incorrect. Correct answer: " + questions_pool[i].answer + "\n");
                    }
                });
            }
        }

        for (auto& t : answer_threads) t.join();

        // Scoreboard
        string scoreboard = "\nScoreboard:\n";
        for (auto& client : clients) {
            if (!client.disconnected) {
                scoreboard += client.name + ": " + to_string(client.score) + " pts\n";
            }
        }
        broadcast(scoreboard);
    }

    // Final Result
    string result = "\nFinal Results:\n";
    sort(clients.begin(), clients.end(), [](const ClientInfo& a, const ClientInfo& b) {
        return a.score > b.score;
    });

    for (size_t i = 0; i < clients.size(); ++i) {
        if (!clients[i].disconnected) {
            result += to_string(i + 1) + ". " + clients[i].name + " - " + to_string(clients[i].score) + " pts\n";
        }
    }

    broadcast(result + "\nGame over!\n");

    for (auto& c : clients)
        if (!c.disconnected) close_socket(c.socket);
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    while (true) {
        clients.clear();
        game_started = false;

        SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            cerr << "Socket creation failed.\n";
            return 1;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            cerr << "Bind failed.\n";
            close_socket(server_socket);
            continue;
        }

        listen(server_socket, MAX_CLIENTS);
        cout << "Server started on port " << PORT << "\n";

        thread([server_socket]() {
            while (!game_started) {
                sockaddr_in client_addr{};
                socklen_t addr_len = sizeof(client_addr);
                SOCKET client_sock = accept(server_socket, (sockaddr*)&client_addr, &addr_len);
                if (client_sock != INVALID_SOCKET) {
                    thread(handle_client, client_sock).detach();
                }
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }).detach();

        // Wait for game start
        while (!game_started) {
            this_thread::sleep_for(chrono::milliseconds(200));
        }

        this_thread::sleep_for(chrono::seconds(START_DELAY_SECONDS));
        run_quiz_round();
        close_socket(server_socket);

        cout << "Restarting server in 5 seconds...\n";
        this_thread::sleep_for(chrono::seconds(5));
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
