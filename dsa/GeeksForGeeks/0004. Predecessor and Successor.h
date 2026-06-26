// // QUESTION
// You are given the root of a BST and an integer key. You need to find the inorder predecessor and successor of the given key. If either predecessor or successor is not found, then set it to NULL.

// Note: In an inorder traversal the number just smaller than the target is the predecessor and the number just greater than the target is the successor. 

// DIFFICULTY
// Medium

// TOPICS
// Binary Search Tree
// Tree
// Data Structures

// TIME AND SPACE COMPLEXITY:

// SOLUTION
#include <bits/stdc++.h>
using namespace std;
/* BST Node
class Node {
   public:
    int data;
    Node *left;
    Node *right;

    Node(int x){
        data = x;
        left = NULL;
        right = NULL;
    }
};
*/

class Solution {
  public:
    vector<Node*> findPreSuc(Node* root, int key) {
        Node* pre = NULL;
        Node* suc = NULL;
        Node* curr = root;
        
        while (curr) {
            if (curr->data == key) {
                // predecessor = rightmost in left subtree
                if (curr->left) {
                    Node* temp = curr->left;
                    while (temp->right) temp = temp->right;
                    pre = temp;
                }
                // successor = leftmost in right subtree
                if (curr->right) {
                    Node* temp = curr->right;
                    while (temp->left) temp = temp->left;
                    suc = temp;
                }
                break;
            }
            else if (curr->data > key) {
                suc = curr;       // possible successor
                curr = curr->left;
            }
            else {
                pre = curr;       // possible predecessor
                curr = curr->right;
            }
        }
        
        return {pre, suc};
    }
};
