#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <random>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>

using namespace std;
using namespace std::chrono;

int shared_score = 0;
mutex score_mutex;
atomic<bool> game_over{false};
string winner = "";

const string player_names[4] = {
    "Alice   ", 
    "Bob     ", 
    "Charlie ", 
    "Diana   "
};

void player_thread(int id, int& local_score_ref) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> points_dist(8, 30);   
    uniform_int_distribution<> delay_dist(120, 280);

    while (!game_over) {
        int points = points_dist(gen);

        // Critical section: update shared score + print
        {
            lock_guard<mutex> lock(score_mutex);

            if (game_over) break;

            shared_score += points;
            local_score_ref += points;        // Add to this player's personal score

            // Print every score update
            cout << player_names[id] 
                 << " scored +" << setw(2) << points 
                 << "  |  Total Score: " << setw(4) << shared_score;

            if (shared_score >= 1000 && !game_over) {
                game_over = true;
                winner = player_names[id];
                cout << "    WINNER! ";
            }
            cout << "\n";
        }

        this_thread::sleep_for(milliseconds(delay_dist(gen)));
    }
}

int main() {
    cout << "==========================================\n";
    cout << "          SHARED SCOREBOARD GAME\n";
    cout << "==========================================\n";
    cout << "4 Players adding to ONE common scoreboard\n";
    cout << "         Reach 1000 points to win!       \n";
    cout << "==========================================\n\n";

    vector<thread> players;
    vector<int> individual_scores(4, 0);   // Store each player's personal score

    // Launch 4 player threads and pass their individual score by reference
    for (int i = 0; i < 4; ++i) {
        players.emplace_back(player_thread, i, ref(individual_scores[i]));
    }

    // Wait for game to end (Important: This was commented out - we need it!)
    while (!game_over) {
        this_thread::sleep_for(100ms);
    }

    // Join all threads
    for (auto& t : players) {
        if (t.joinable()) {
            t.join();
        }
    }

    // ====================== FINAL RESULTS ======================
    cout << "\n" << string(50, '=') << "\n";
    cout << "                  GAME OVER\n";
    cout << string(50, '=') << "\n\n";

    cout << "Final Shared Score : " << shared_score << "\n\n";
    cout << "Individual Scores:\n";
    cout << "-----------------\n";

    for (int i = 0; i < 4; ++i) {
        cout << player_names[i] 
             << " scored " << individual_scores[i] 
             << " points\n";
    }

    transform(winner.begin(), winner.end(), winner.begin(), ::toupper);
    cout << "\nWinner: " << winner << " \n";

    return 0;
}