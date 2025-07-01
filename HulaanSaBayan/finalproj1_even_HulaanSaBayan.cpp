#include <iostream>
#include <ctime>
#include <thread>
#include <string>
#include <limits>
#include <mutex>
#include <vector>
#include <cctype>
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
    bool phoneAFriend;
    bool option5050;
    bool askTheAudience;
    int id;
    char answer;

public:
    Player() {
        name = "";
        score = 0;
        answer = ' ';
        id = 0;
    }

    Player(string passedName, int id) {
        name = passedName;
        score = 0;
        answer = ' ';
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

    void setLatestAnswer(char playerAnswer) {
        answer = playerAnswer;
    }

    char consumeLatestAnswer() {
        char temp = answer;
        answer = ' ';
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
    vector<string> choices;
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

        if (tokens.size() == 5) {
            Question q;
            q.question = tokens[0];
            q.correct = tokens[1];
            q.choices = {tokens[1], tokens[2], tokens[3], tokens[4]};

            shuffle(q.choices.begin(), q.choices.end(), default_random_engine(random_device{}()));

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

bool checkAnswer(Player& player, char correctAnswer) {
    char playerAnswer = player.consumeLatestAnswer();
    if (playerAnswer == correctAnswer) {
        player.updateScore(10);
        return true;
    } else {
        return false;
    }
}

void simulateRound(Player& player, char correctAnswer) {
    char choice;
    future<bool> result;

    {
        lock_guard<mutex> lock(mtx);
        cout << player.getName() << ", enter answer (A, B, C, or D): " << flush;
        choice = 'A' + (rand() % 4);
        this_thread::sleep_for(std::chrono::seconds(4 + (rand() % 8)));
        cout << choice << endl;
        player.setLatestAnswer(choice);
        result = async(launch::async, checkAnswer, ref(player), correctAnswer);
    }

    gameBarrier->arrive_and_wait();
    
    {
        lock_guard<mutex> lock(mtx);
        if(revealAnswer){
            cout << "\nCorrect Answer: " << correctAnswer << "\n\n";
            revealAnswer = false;
        }
    }

    {
        lock_guard<mutex> lock(mtx);
        
        cout << player.getName() << " answered: " << choice << endl;
        if (result.get()) {
            cout << "Correct!\n";
        } else {
            cout << "Incorrect!\n";
        }
    }
}

void simulateRoundManual(Player& player, char correctAnswer) {
    char choice;
    future<bool> result;

    {
        lock_guard<mutex> lock(mtx);
        cout << player.getName() << ", enter answer (A, B, C, or D): ";
        cin >> choice;
        choice = toupper(choice);
        player.setLatestAnswer(choice);
        result = async(launch::async, checkAnswer, ref(player), correctAnswer);
    }

    gameBarrier->arrive_and_wait();
    
    {
        lock_guard<mutex> lock(mtx);
        if(revealAnswer){
            cout << "\nCorrect Answer: " << correctAnswer << "\n\n";
            revealAnswer = false;
        }
    }

    {
        lock_guard<mutex> lock(mtx);
        
        cout << player.getName() << " answered: " << choice << endl;
        if (result.get()) {
            cout << "Correct!\n";
        } else {
            cout << "Incorrect!\n";
        }
    }
}

void simulateGame(vector<Player>& quizPlayers, int menuChoice) {
    
    int numPlayers = quizPlayers.size();
    vector<Question> allQuestions = loadQuestions("questions.txt");
    
    shuffle(allQuestions.begin(), allQuestions.end(), default_random_engine(random_device{}()));
    vector<Question> gameQuestions(allQuestions.begin(), allQuestions.begin() + 5);
    
    
    for(int i = 0; i < 5; i++){
        Question& q = gameQuestions[i];
        revealAnswer = true;
        
        cout << "===== ROUND " << i + 1 << " =====\n";
        cout << q.question << "\n";
        
        char correctAnswer = ' ';
        for (int i = 0; i < 4; ++i) {
            char label = 'A' + i;
            cout << label << ") " << q.choices[i] << "\n";
            if (q.choices[i] == q.correct) {
                correctAnswer = label;
            }
        }
        cout << endl;
        
        vector<thread> gameThreads;
        
        if(menuChoice == 1){
            
            for (int i = 0; i < numPlayers; ++i) {
                gameThreads.emplace_back(simulateRound, ref(quizPlayers[i]), correctAnswer);
            }
        
            for (auto& t : gameThreads) {
                t.join();
            }
            
        } else if(menuChoice == 2){
            for (int i = 0; i < numPlayers; ++i) {
                gameThreads.emplace_back(simulateRoundManual, ref(quizPlayers[i]), correctAnswer);
            }
        
            for (auto& t : gameThreads) {
                t.join();
            }
        }
        
        this_thread::sleep_for(std::chrono::seconds(5));
    
        cout << "\nCurrent ScoreBoard:\n";
        for (int i = 0; i < numPlayers; i++) {
            cout << quizPlayers[i].getName() << " Score: " << quizPlayers[i].getScore() << endl;
        }
    
        cout << "\n\n";
    }
    
        /* final results */
    int maxScore = -99;
    for (auto& p: quizPlayers) {
        maxScore = max(maxScore, p.getScore());
    }
    int count = 0;
    for (auto& p: quizPlayers) {
        if (p.getScore() == maxScore) {
            count++;
        }
    }

    cout << "\n=========== FINAL RESULTS ===========\n";
    for (auto& p: quizPlayers) {
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

    cout << "Welcome to a Filipino Trivia Game\n\n";
    
    cout << "1. Simulated\n";
    cout << "2. Manual\n\n";
    
    int menuChoice;
    menuChoice = inputValidationMenu(1, 2, "Enter here: ");
    
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
        
    simulateGame(quizPlayers, menuChoice);
    

    

    return 0;
}
