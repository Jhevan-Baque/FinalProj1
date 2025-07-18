// server.cpp - Quiz Game Server (Auto-Start After 2 Players + Reset Loop)

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <thread>
#include <mutex>
#include <atomic>
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
#endif

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 2048
#define MAX_CLIENTS 50
#define START_AFTER_SECONDS 30
#define MIN_PLAYERS 2

struct ClientInfo {
    SOCKET socket;
    string name;
    int score = 0;
    bool disconnected = false;
};

struct Question {
    string question;
    string correct;
};

mutex clients_mutex;
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

void send_to_client(SOCKET sock, const string& msg) {
    send(sock, msg.c_str(), static_cast<int>(msg.size()), 0);
}

void broadcast(const vector<ClientInfo>& clients, const string& message) {
    for (const auto& client : clients) {
        if (!client.disconnected) {
            send_to_client(client.socket, message);
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

vector<ClientInfo> clients;
atomic<bool> game_started{false};

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE]{};
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        close_socket(client_socket);
        return;
    }

    buffer[bytes_received] = '\0';
    string player_name(buffer);

    {
        lock_guard<mutex> lock(clients_mutex);
        clients.push_back({client_socket, player_name});
        cout << player_name << " joined (" << clients.size() << " players).\n";

        send_to_client(client_socket, "Welcome " + player_name + "! Waiting for others...\n");
    }

    if (clients.size() == MIN_PLAYERS && !game_started.exchange(true)) {
        thread([] {
            cout << "Minimum players reached. Waiting 30 seconds for more players...\n";
            for (int i = START_AFTER_SECONDS; i > 0; --i) {
                string countdown_msg = "Game starting in " + to_string(i) + " seconds...\n";
                broadcast(clients, countdown_msg);
                this_thread::sleep_for(chrono::seconds(1));
            }
            broadcast(clients, "Game starting now...\n");
            cout << "Game starting...\n";
        }).detach();
    }
}

void run_quiz_round() {
    vector<Question> questions = questions_pool;
    shuffle(questions.begin(), questions.end(), mt19937(random_device{}()));
    int num_questions = min(5, (int)questions.size());

    for (int i = 0; i < num_questions; ++i) {
        string qtext = "\nQuestion " + to_string(i + 1) + ": " + questions[i].question + "\nAnswer: ";
        broadcast(clients, qtext);

        vector<thread> threads;

        {
            lock_guard<mutex> lock(clients_mutex);
            for (auto& client : clients) {
                threads.emplace_back([&client, &q = questions[i]]() {
                    char buffer[BUFFER_SIZE]{};
                    int len = recv(client.socket, buffer, BUFFER_SIZE - 1, 0);
                    if (len <= 0) {
                        client.disconnected = true;
                        close_socket(client.socket);
                        return;
                    }

                    buffer[len] = '\0';
                    string answer = buffer;
                    string correct = q.correct;

                    transform(answer.begin(), answer.end(), answer.begin(), ::tolower);
                    transform(correct.begin(), correct.end(), correct.begin(), ::tolower);

                    if (answer == correct || answer.find(correct) != string::npos) {
                        client.score += 10;
                        send_to_client(client.socket, "Correct! (+10 pts)\n");
                    } else {
                        send_to_client(client.socket, "Wrong. Correct answer: " + q.correct + "\n");
                    }
                });
            }
        }

        for (auto& t : threads) t.join();

        string scoreboard = "\nSCOREBOARD:\n";
        for (const auto& client : clients) {
            if (!client.disconnected) {
                scoreboard += client.name + ": " + to_string(client.score) + " pts\n";
            }
        }
        broadcast(clients, scoreboard);
    }

    string result = "\nFINAL RESULTS:\n";
    vector<ClientInfo> finalists;

    for (auto& c : clients)
        if (!c.disconnected) finalists.push_back(c);

    sort(finalists.begin(), finalists.end(),
         [](const ClientInfo& a, const ClientInfo& b) { return a.score > b.score; });

    for (size_t i = 0; i < finalists.size(); i++) {
        result += to_string(i + 1) + ". " + finalists[i].name + " - " + to_string(finalists[i].score) + " pts\n";
    }

    if (!finalists.empty()) {
        int top = finalists[0].score;
        vector<string> winners;
        for (auto& c : finalists)
            if (c.score == top) winners.push_back(c.name);

        if (winners.size() == 1)
            result += "\nWinner: " + winners[0] + "\n";
        else {
            result += "\nIt's a tie between: ";
            for (size_t i = 0; i < winners.size(); ++i) {
                result += winners[i];
                if (i != winners.size() - 1) result += ", ";
            }
            result += "\n";
        }
    }

    broadcast(clients, result + "\nGame over\nThank you for playing!\n");

    for (auto& c : clients) {
        if (!c.disconnected) {
            close_socket(c.socket);
        }
    }
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
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);

        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Bind failed.\n";
            close_socket(server_socket);
            continue;
        }

        listen(server_socket, MAX_CLIENTS);
        cout << "\nServer listening on port " << PORT << ". Waiting for players...\n";

        thread([server_socket]() {
            while (!game_started) {
                sockaddr_in client_addr{};
                socklen_t len = sizeof(client_addr);
                SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &len);
                if (client_socket >= 0) {
                    thread(handle_client, client_socket).detach();
                }
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }).detach();

        while (!game_started) this_thread::sleep_for(chrono::milliseconds(100));

        this_thread::sleep_for(chrono::seconds(START_AFTER_SECONDS));
        run_quiz_round();
        close_socket(server_socket);

        cout << "\nRestarting server...\n";
        this_thread::sleep_for(chrono::seconds(3));
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
