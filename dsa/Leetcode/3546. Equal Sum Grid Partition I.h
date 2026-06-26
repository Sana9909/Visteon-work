// QUESTION
// You are given an m x n matrix grid of positive integers. Your task is to determine if it is possible to make either one horizontal or one vertical cut on the grid such that:

// Each of the two resulting sections formed by the cut is non-empty.
// The sum of the elements in both sections is equal.
// Return true if such a partition exists; otherwise return false.

// DIFFICULTY
// Medium

// TOPICS
// Senior
// Array
// Matrix
// Enumeration
// Prefix Sum
// Weekly Contest 449

// TIME AND SPACE COMPLEXITY:

// SOLUTION
#include <vector>
using namespace std;
class Solution {
public:
    bool canPartitionGrid(vector<vector<int>>& grid) {
        int m = grid.size(), n = grid[0].size();

        vector<long long> rowSum(m, 0), colSum(n, 0);
        long long total = 0;

        // Compute rowSum, colSum, total
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++) {
                rowSum[i] += grid[i][j];
                colSum[j] += grid[i][j];
                total += grid[i][j];
            }
        }

        if (total % 2) return false;

        if (check(rowSum, total)) return true;

        if (check(colSum, total)) return true;

        return false;
    }

    bool check(vector<long long>& arr, long long total) {
        long long left = arr[0];
        long long right = total - left;

        for (int i = 1; i < arr.size(); i++) {
            if (left == right) return true;
            else if(left>right) return false;
            left += arr[i];
            right -= arr[i];
        }
        
        return false;
    }
};