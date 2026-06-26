// QUESTION
// Burning Tree
// Difficulty: HardAccuracy: 53.53%Submissions: 137K+Points: 8
// Given the root of a binary tree and a target node, determine the minimum time required to burn the entire tree if the target node is set on fire. In one second, the fire spreads from a node to its left child, right child, and parent.
// Note: The tree contains unique values.

// DIFFICULTY
//Hard

// TOPICS
// TreeBFS
// Data Structures
// Algorithms

// TIME AND SPACE COMPLEXITY:

// SOLUTION
#include <unordered_map>
#include <queue>
#include <unordered_set>

using namespace std;

/*
class Node {
public:
    int data;
    Node *left;
    Node *right;
    Node(int val) {
        data = val;
        left = right = NULL;
    }
};
*/

class Solution {
public:
    // Helper to map parents and find the target node pointer
    Node* mapParents(Node* root, int target, unordered_map<Node*, Node*>& parentMap) {
        queue<Node*> q;
        q.push(root);
        Node* targetNode = nullptr;
        
        while (!q.empty()) {
            Node* curr = q.front();
            q.pop();
            
            if (curr->data == target) targetNode = curr;
            
            if (curr->left) {
                parentMap[curr->left] = curr;
                q.push(curr->left);
            }
            if (curr->right) {
                parentMap[curr->right] = curr;
                q.push(curr->right);
            }
        }
        return targetNode;
    }

    int minTime(Node* root, int target) {
        unordered_map<Node*, Node*> parentMap;
        Node* targetNode = mapParents(root, target, parentMap);
        
        if (!targetNode) return 0;

        // BFS to simulate burning
        queue<Node*> q;
        q.push(targetNode);
        
        unordered_map<Node*, bool> visited;
        visited[targetNode] = true;
        
        int time = 0;
        
        while (!q.empty()) {
            int size = q.size();
            bool spread = false;
            
            for (int i = 0; i < size; i++) {
                Node* curr = q.front();
                q.pop();
                
                // Spread to Left Child
                if (curr->left && !visited[curr->left]) {
                    visited[curr->left] = true;
                    q.push(curr->left);
                    spread = true;
                }
                // Spread to Right Child
                if (curr->right && !visited[curr->right]) {
                    visited[curr->right] = true;
                    q.push(curr->right);
                    spread = true;
                }
                // Spread to Parent
                if (parentMap[curr] && !visited[parentMap[curr]]) {
                    visited[parentMap[curr]] = true;
                    q.push(parentMap[curr]);
                    spread = true;
                }
            }
            // Only increment time if the fire actually spread to a new node
            if (spread) time++;
        }
        
        return time;
    }
};
