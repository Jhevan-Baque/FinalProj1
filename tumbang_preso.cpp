#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <set>
#include <thread>
#include <future>
#include <mutex>

using namespace std;

constexpr int NUM_PLAYERS = 3;
constexpr int NUM_ROUNDS = 3;

random_device rd;
mt19937 gen(rd());
uniform_int_distribution<> player_dist(0, NUM_PLAYERS - 1);

// handles the function if can is knocked-down (returns true if knocked)
bool simulate_can_knockdown() {
    promise<bool> p;
    auto fut = p.get_future();
    thread t([&p](){
        uniform_real_distribution<> prob_dist(0.0, 1.0);
        this_thread::sleep_for(chrono::milliseconds(100));
        p.set_value(prob_dist(gen) < 0.5);
    });
    t.join();
    return fut.get();
}
// handles the function tag success (returns true if tagged)
bool simulate_tag_success() {
    promise<bool> p;
    auto fut = p.get_future();
    thread t([&p](){
        uniform_real_distribution<> tag_prob(0.0, 1.0);
        this_thread::sleep_for(chrono::milliseconds(100));
        p.set_value(tag_prob(gen) < 0.7);
    });
    t.join();
    return fut.get();
}

string get_action_from_user(int player_id, const string& prompt, const vector<string>& valid_actions) {
    string action;
    while (true) {
        cout << "Player " << (player_id + 1) << ", " << prompt << flush;
        getline(cin, action);
        transform(action.begin(), action.end(), action.begin(), ::tolower);
        if (find(valid_actions.begin(), valid_actions.end(), action) != valid_actions.end()) {
            return action;
        } else {
            cout << "Invalid input. Please enter a valid move: ";
            for (const auto& a : valid_actions) cout << "'" << a << "' ";
            cout << "\n";
        }
    }
}

int main() {
    cout << "Welcome to Tumbang Preso 2.0!\n";
    cout << "There are " << NUM_PLAYERS << " players and " << NUM_ROUNDS << " rounds.\n";
    int tayaa = player_dist(gen);
    set<int> needs_to_retrieve; // players who need to retrieve their slipper
    for (int round = 0; round < NUM_ROUNDS; ++round) {
        bool can_knocked = false;
        int can_knocked_by = -1;
        cout << "\n--- Round " << (round + 1) << " ---\n";
        cout << "Player " << (tayaa + 1) << " is the taya (It/Tagger) and guards the can!\n";
        // Throwing Phase
        vector<string> round_actions(NUM_PLAYERS, "none");
        vector<bool> threw(NUM_PLAYERS, false);
        for (int i = 0; i < NUM_PLAYERS; ++i) {
            if (can_knocked) {
                round_actions[i] = "none";
                continue;
            }
            if (i != tayaa && needs_to_retrieve.count(i) == 0) {
                string action = get_action_from_user(i, "do you want to 'throw' your slipper or 'wait'? ", {"throw", "wait"});
                round_actions[i] = action;
                if (action == "throw") {
                    threw[i] = true;
                    if (simulate_can_knockdown()) {
                        can_knocked = true;
                        can_knocked_by = i;
                    }
                }
            } else {
                round_actions[i] = "none";
            }
        }
        // Print The Results of Throws.
        for (int i = 0; i < NUM_PLAYERS; ++i) {
            if (round_actions[i] == "throw") {
                if (can_knocked && can_knocked_by == i) {
                    cout << "Player " << (i + 1) << " knocked down the can!\n";
                } else {
                    cout << "Player " << (i + 1) << " missed the can.\n";
                }
            } else if (round_actions[i] == "wait") {
                cout << "Player " << (i + 1) << " waits.\n";
            }
        }
        for (int i = 0; i < NUM_PLAYERS; ++i) if (threw[i]) needs_to_retrieve.insert(i);
        // Retrieving Phase
        vector<int> running_this_round;
        vector<int> hiding_this_round;
        if (can_knocked && !needs_to_retrieve.empty()) {
            cout << "\nThe can is down! Taya (Player " << (tayaa + 1) << ") must reset the can before tagging anyone.\n";
            cout << "Taya is resetting the can...\n";
            for (int i = 0; i < NUM_PLAYERS; ++i) {
                if (needs_to_retrieve.count(i) && i != tayaa) {
                    string action = get_action_from_user(i, "do you want to 'run' (retrieve your slipper) or 'hide'? ", {"run", "hide"});
                    if (action == "run") running_this_round.push_back(i);
                    else hiding_this_round.push_back(i);
                }
            }
            if (!running_this_round.empty()) {
                cout << "Players running to retrieve their slippers: ";
                for (int r : running_this_round) cout << (r + 1) << " ";
                cout << "\n";
                cout << "Taya (Player " << (tayaa + 1) << ") can try to tag a runner.\n";
                cout << "Enter the number of the player to tag (or 0 to skip): " << flush;
                int tag_choice = -1;
                while (true) {
                    string input;
                    getline(cin, input);
                    try {
                        tag_choice = stoi(input);
                    } catch (...) {
                        tag_choice = -1;
                    }
                    bool valid = (tag_choice == 0);
                    for (int r : running_this_round) {
                        if (tag_choice == r + 1) { valid = true; break; }
                    }
                    if (valid) break;
                    cout << "Invalid choice. Enter a runner's number or 0 to skip: " << flush;
                }
                if (tag_choice != 0) {
                    if (simulate_tag_success()) {
                        cout << "Player " << tag_choice << " was tagged and is now the new taya!\n";
                        tayaa = tag_choice - 1;
                        // All other runners escape
                        for (int r : running_this_round) {
                            if (r != tag_choice - 1) {
                                cout << "Player " << (r + 1) << " retrieved their slipper safely.\n";
                                needs_to_retrieve.erase(r);
                            }
                        }
                    } else {
                        cout << "Player " << tag_choice << " escaped the tag and retrieved their slipper!\n";
                        needs_to_retrieve.erase(tag_choice - 1);
                        // All other runners escape too
                        for (int r : running_this_round) {
                            if (r != tag_choice - 1) {
                                cout << "Player " << (r + 1) << " retrieved their slipper safely.\n";
                                needs_to_retrieve.erase(r);
                            }
                        }
                    }
                } else {
                    cout << "Taya chose not to tag anyone.\n";
                    for (int r : running_this_round) {
                        cout << "Player " << (r + 1) << " retrieved their slipper safely.\n";
                        needs_to_retrieve.erase(r);
                    }
                }
            } else {
                cout << "No one is running to retrieve a slipper this round.\n";
            }
        } else if (can_knocked) {
            cout << "\nThe can is down, but no one needs to retrieve a slipper.\n";
        } else {
            cout << "\nThe can was not knocked down. Taya remains the same.\n";
        }
        cout << "All slippers are returned (if retrieved), and the can is reset for the next round.\n";
        if (needs_to_retrieve.size() == NUM_PLAYERS - 1) {
            // All slippers are on the field
            cout << "All slippers are on the field! Taya (Player " << (tayaa + 1) << "), do you want to bait the can? (yes/no): ";
            string bait_choice;
            getline(cin, bait_choice);
            if (bait_choice == "yes") {
                vector<int> running_this_round;
                vector<int> hiding_this_round;
                for (int i = 0; i < NUM_PLAYERS; ++i) {
                    if (needs_to_retrieve.count(i) && i != tayaa) {
                        string action = get_action_from_user(i, "do you want to 'run' (retrieve your slipper) or 'hide'? ", {"run", "hide"});
                        if (action == "run") running_this_round.push_back(i);
                        else hiding_this_round.push_back(i);
                    }
                }
                if (!running_this_round.empty()) {
                    cout << "Players running to retrieve their slippers: ";
                    for (int r : running_this_round) cout << (r + 1) << " ";
                    cout << "\n";
                    cout << "Taya (Player " << (tayaa + 1) << ") can try to tag a runner.\n";
                    cout << "Enter the number of the player to tag (or 0 to skip): " << flush;
                    int tag_choice = -1;
                    while (true) {
                        string input;
                        getline(cin, input);
                        try {
                            tag_choice = stoi(input);
                        } catch (...) {
                            tag_choice = -1;
                        }
                        bool valid = (tag_choice == 0);
                        for (int r : running_this_round) {
                            if (tag_choice == r + 1) { valid = true; break; }
                        }
                        if (valid) break;
                        cout << "Invalid choice. Enter a runner's number or 0 to skip: " << flush;
                    }
                    if (tag_choice != 0) {
                        if (simulate_tag_success()) {
                            cout << "Player " << tag_choice << " was tagged and is now the new taya!\n";
                            tayaa = tag_choice - 1;
                            // All other runners escape
                            for (int r : running_this_round) {
                                if (r != tag_choice - 1) {
                                    cout << "Player " << (r + 1) << " retrieved their slipper safely.\n";
                                    needs_to_retrieve.erase(r);
                                }
                            }
                        } else {
                            cout << "Player " << tag_choice << " escaped the tag and retrieved their slipper!\n";
                            needs_to_retrieve.erase(tag_choice - 1);
                            // All other runners escape too
                            for (int r : running_this_round) {
                                if (r != tag_choice - 1) {
                                    cout << "Player " << (r + 1) << " retrieved their slipper safely.\n";
                                    needs_to_retrieve.erase(r);
                                }
                            }
                        }
                    } else {
                        cout << "Taya chose not to tag anyone.\n";
                        for (int r : running_this_round) {
                            cout << "Player " << (r + 1) << " retrieved their slipper safely.\n";
                            needs_to_retrieve.erase(r);
                        }
                    }
                } else {
                    cout << "No one is running to retrieve a slipper this round.\n";
                }
            } else {
                cout << "Taya chose not to bait. No one can retrieve their slipper this round.\n";
                // Skip retrieval phase
            }
        }
    }
    cout << "\nGame over! Thanks for playing.\n";
    return 0;
} 