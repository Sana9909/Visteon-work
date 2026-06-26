// QUESTION
// You're given a binary tree. Your task is to find the size of the largest subtree within this binary tree that also satisfies the properties of a Binary Search Tree (BST). The size of a subtree is defined as the number of nodes it contains.

// Note: A subtree of the binary tree is considered a BST if for every node in that subtree, the left child is less than the node, and the right child is greater than the node, without any duplicate values in the subtree.

// DIFFICULTY
// Medium

// TOPICS
// Binary Search Tree
// Tree
// Data Structures
// Sorting

// TIME AND SPACE COMPLEXITY:

// SOLUTION
#include <bits/stdc++.h>
using namespace std;

// class Node {
// public:
//     int data;
//     Node* left;
//     Node* right;
//     Node(int x) {
//         data = x;
//         left = right = nullptr;
//     }
// };

struct Info {
    bool isBST;
    int size;
    int minVal;
    int maxVal;
};

class Solution {
public:
    int maxSize;

    Info solve(Node* root) {
        if (!root) {
            return {true, 0, INT_MAX, INT_MIN};
        }

        Info left = solve(root->left);
        Info right = solve(root->right);

        Info curr;
        curr.size = left.size + right.size + 1;
        curr.minVal = min(root->data, left.minVal);
        curr.maxVal = max(root->data, right.maxVal);

        if (left.isBST && right.isBST &&
            root->data > left.maxVal && root->data < right.minVal) {
            curr.isBST = true;
            maxSize = max(maxSize, curr.size);
        } else {
            curr.isBST = false;
        }

        return curr;
    }

    int largestBst(Node* root) {
        maxSize = 0;
        solve(root);
        return maxSize;
    }
};
