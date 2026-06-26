#include<iostream>
#include<thread>
#include<mutex>
#include<atomic>
#include<random>
#include<chrono>
#include<vector>
#include<string>
#include<algorithm>

using namespace std;
using namespace std::chrono;

int sharedScore=0;
atomic<bool> gameOver{false};
mutex scoreMutex;
string winner="";

vector<string> playerNames = {"A", "B", "C", "D"};

void playerThread(int id,int &localScoreRef){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> pointsDist(8,30);
    uniform_int_distribution<> delayDist(120,280);

    while(!gameOver){
        int points = pointsDist(gen);

        {
            lock_guard<mutex> lock(scoreMutex);
            if(gameOver) break;

            sharedScore += points;
            localScoreRef += points;

            cout << playerNames[id] << " scored +" << points 
                 << "  |  Total Score: " << sharedScore;

            if(sharedScore >= 1000 && !gameOver){
                gameOver = true;
                winner = playerNames[id];
                cout << "    WINNER! ";
            }
            cout << "\n";
        }
        this_thread::sleep_for(milliseconds(delayDist(gen)));
    }
}

int main(){
    vector<thread> players;
    vector<int> localScores(4,0);

    for(int i=0;i<4;i++){
        players.emplace_back(playerThread,i,ref(localScores[i]));
    }

    for(auto& p : players){
        p.join();
    }

    cout << "\n\nIndividual Scores:\n";
    cout << "-----------------\n";

    for (int i = 0; i < 4; ++i) {
        cout << playerNames[i] 
             << " scored " << localScores[i] 
             << " points\n";
    }

    cout << "\n\nFinal Winner: " << winner << endl;
    cout<<endl;
    return 0;

}