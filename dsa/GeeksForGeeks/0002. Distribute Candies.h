// QUESTION
// You are given the root of a binary tree with n nodes, where each node contains a certain number of candies, and the total number of candies across all nodes is n. In one move, you can select any two adjacent nodes and transfer one candy from one node to the other. The transfer can occur between a parent and child in either direction.

// The task is to determine the minimum number of moves required to ensure that every node in the tree has exactly one candy.

// Note: The testcases are framed such that it is always possible to achieve a configuration in which every node has exactly one candy, after some moves.

// DIFFICULTY
// Hard

// TOPICS
// Tree
// Data Structures

// TIME AND SPACE COMPLEXITY:

// SOLUTION

#include <cmath>

class Node {
public:
    int data;
    Node* left;
    Node* right;

    Node(int x) {
        data = x;
        left = right = nullptr;
    }
};


class Solution {
  public:
    int moves;

    int dfs(Node* root) {
        if (!root) return 0;

        int left = dfs(root->left);
        int right = dfs(root->right);

        // Count moves needed for left and right subtrees
        moves += abs(left) + abs(right);

        // Return balance of current node
        return root->data + left + right - 1;
    }

    int distCandy(Node* root) {
        moves = 0;
        dfs(root);
        return moves;
    }
};


