#include <iostream>
#include <ctime>
#include <thread>
#include <string>
#include <limits>
#include <mutex>
#include <vector>
#include <latch>
#include <barrier>
#include <future>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <chrono>

using namespace std;

mutex mtx;
latch* globalLatch = nullptr;
barrier<>* gameBarrier = nullptr;
atomic<bool> revealAnswer(false);

class Player {
private:
    string name;
    int score;
    string answer;
    int id;

public:
    Player() {
        name = "";
        score = 0;
        answer = "";
        id = 0;
    }

    Player(string passedName, int id) {
        name = passedName;
        score = 0;
        answer = "";
        this->id = id;
    }

    string getName() {
        return name;
    }

    int getId() {
        return id;
    }

    void updateScore(int scoreToAdd) {
        score += scoreToAdd;
    }

    int getScore() {
        return score;
    }

    void setLatestAnswer(string playerAnswer) {
        answer = playerAnswer;
    }

    string consumeLatestAnswer() {
        string temp = answer;
        answer = "";
        return temp;
    }
};

int inputValidationMenu(int min, int max, string message) {
    int choice = 0;
    while (true) {
        cout << message;
        if (!(cin >> choice)) {
            cout << "\nInvalid input! Please enter a valid positive integer\n";
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }
        if (choice < min || choice > max) {
            cout << "\nInvalid input! Please enter a number within the allowed limit\n";
            continue;
        }
        break;
    }
    return choice;
}

struct Question {
    string question;
    string correct;
};

vector<Question> loadQuestions(const string& filename) {
    vector<Question> questions;
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string part;
        vector<string> tokens;

        while (getline(ss, part, ',')) {
            tokens.push_back(part);
        }

        if (tokens.size() >= 2) {
            Question q;
            q.question = tokens[0];
            q.correct = tokens[1];
            questions.push_back(q);
        }
    }

    return questions;
}

void waitAllPlayers(int threadIndex, vector<Player>& quizPlayers) {
    string name;
    {
        lock_guard<mutex> lock(mtx);
        cout << "Enter name for Player " << (threadIndex + 1) << ": ";
        getline(cin, name);
        string playerName = "Player " + to_string(threadIndex + 1) + ", " + name;
        quizPlayers[threadIndex] = Player(playerName, threadIndex + 1);
    }

    globalLatch->count_down();
    globalLatch->wait();
    {
        lock_guard<mutex> lock(mtx);
        cout << "Player " << name << ", ID: " << (threadIndex + 1) << " has joined.\n";
    }
}

bool checkAnswer(Player& player, const string& correctAnswer) {
    string playerAnswer = player.consumeLatestAnswer();
    transform(playerAnswer.begin(), playerAnswer.end(), playerAnswer.begin(), ::tolower);
    string correctLower = correctAnswer;
    transform(correctLower.begin(), correctLower.end(), correctLower.begin(), ::tolower);

    return playerAnswer == correctLower;
}

void simulateRound(Player& player, const string& correctAnswer) {
    string choice;
    future<bool> result;

    {
        lock_guard<mutex> lock(mtx);
        cout << player.getName() << ", enter your answer: ";
        getline(cin, choice);
        player.setLatestAnswer(choice);
        result = async(launch::async, checkAnswer, ref(player), correctAnswer);
    }

    gameBarrier->arrive_and_wait();

    {
        lock_guard<mutex> lock(mtx);
        if (revealAnswer) {
            cout << "\nCorrect Answer: " << correctAnswer << "\n\n";
            revealAnswer = false;
        }
    }

    {
        lock_guard<mutex> lock(mtx);
        cout << player.getName() << " answered: " << choice << endl;
        if (result.get()) {
            cout << "Correct!\n";
            player.updateScore(10);
        } else {
            cout << "Incorrect!\n";
        }
    }
}

void simulateGame(vector<Player>& quizPlayers) {
    int numPlayers = quizPlayers.size();
    vector<Question> allQuestions = loadQuestions("questions.txt");

    shuffle(allQuestions.begin(), allQuestions.end(), default_random_engine(random_device{}()));
    vector<Question> gameQuestions(allQuestions.begin(), allQuestions.begin() + min(5, (int)allQuestions.size()));

    for (int i = 0; i < gameQuestions.size(); i++) {
        Question& q = gameQuestions[i];
        revealAnswer = true;

        cout << "===== ROUND " << i + 1 << " =====\n";
        cout << q.question << "\n";
        cout << "(Type your answer):\n";

        vector<thread> gameThreads;

        for (int i = 0; i < numPlayers; ++i) {
            gameThreads.emplace_back(simulateRound, ref(quizPlayers[i]), q.correct);
        }

        for (auto& t : gameThreads) {
            t.join();
        }

        this_thread::sleep_for(std::chrono::seconds(5));

        cout << "\nCurrent ScoreBoard:\n";
        for (int i = 0; i < numPlayers; i++) {
            cout << quizPlayers[i].getName() << " Score: " << quizPlayers[i].getScore() << endl;
        }

        cout << "\n\n";
    }

    int maxScore = -99;
    for (auto& p : quizPlayers) {
        maxScore = max(maxScore, p.getScore());
    }
    int count = 0;
    for (auto& p : quizPlayers) {
        if (p.getScore() == maxScore) {
            count++;
        }
    }

    cout << "\n=========== FINAL RESULTS ===========\n";
    for (auto& p : quizPlayers) {
        cout << p.getName() << " — " << p.getScore() << " pts";
        if (p.getScore() == maxScore && count == 1) {
            cout << "  <-- WINNER!";
        } else if (p.getScore() == maxScore && count > 1) {
            cout << "  <-- IT'S A TIE!";
        }
        cout << "\n";
    }
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    cout << "Welcome to Hulaan ng Bayan\n\n";

    int numPlayers = inputValidationMenu(1, 9999, "Enter number of players: ");
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    vector<thread> playerThreads;
    vector<Player> quizPlayers(numPlayers);

    latch latchObj(numPlayers);
    globalLatch = &latchObj;

    barrier<> barrierObj(numPlayers);
    gameBarrier = &barrierObj;

    cout << "\nWaiting for all players to join...\n\n";

    for (int i = 0; i < numPlayers; ++i) {
        playerThreads.emplace_back(waitAllPlayers, i, ref(quizPlayers));
    }

    for (auto& t : playerThreads) {
        t.join();
    }

    cout << "\nAll players are ready. Starting game now...\n\n";

    simulateGame(quizPlayers);

    return 0;
}
