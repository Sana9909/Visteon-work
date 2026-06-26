// // QUESTION
// You are given an undirected graph, which has tree characteristics with V vertices numbered from 0 to V-1 and E edges, represented as a 2D array edges[][], where each element edges[i] = [u, v] represents an edge from vertex u to v.

// You can choose any vertex as the root of the tree. Your task is to find all the vertices that, when chosen as the root, result in the minimum possible height of the tree.

// Note: The height of a rooted tree is defined as the maximum number of edges on the path from the root to any leaf node.

// DIFFICULTY
// Medium

// TOPICS
// Graph
// topological-sort
// BFS

// TIME AND SPACE COMPLEXITY:

// SOLUTION
#include <vector>
#include <queue>
using namespace std;
class Solution {
  public:
    vector<int> minHeightRoot(int V, vector<vector<int>>& edges) {
        if (V == 1) return {0};  // single node case
        
        // adjacency list
        vector<vector<int>> adj(V);
        vector<int> degree(V, 0);
        
        for (auto &e : edges) {
            int u = e[0], v = e[1];
            adj[u].push_back(v);
            adj[v].push_back(u);
            degree[u]++;
            degree[v]++;
        }
        
        // initialize leaves
        queue<int> q;
        for (int i = 0; i < V; i++) {
            if (degree[i] == 1) q.push(i);
        }
        
        vector<int> res;
        while (!q.empty()) {
            int size = q.size();
            res.clear();  // last layer will be answer
            
            for (int i = 0; i < size; i++) {
                int node = q.front(); q.pop();
                res.push_back(node);
                
                for (int nei : adj[node]) {
                    degree[nei]--;
                    if (degree[nei] == 1) q.push(nei);
                }
            }
        }
        
        return res;
    }
};

