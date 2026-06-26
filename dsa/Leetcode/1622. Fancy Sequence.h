// QUESTION:
// 1622. Fancy Sequence

// Write an API that generates fancy sequences using the append, addAll, and multAll operations.
// Implement the Fancy class:
// Fancy() Initializes the object with an empty sequence.
// void append(val) Appends an integer val to the end of the sequence.
// void addAll(inc) Increments all existing values in the sequence by an integer inc.
// void multAll(m) Multiplies all existing values in the sequence by an integer m.
// int getIndex(idx) Gets the current value at index idx (0-indexed) of the sequence modulo 109 + 7. If the index is greater or equal than the length of the sequence, return -1.

//DIFFICULTY
//Hard

//TOPICS
// Principal
// Math
// Design
// Segment Tree
// Biweekly Contest 37

// TIME AND SPACE COMPLEXITY:
// Operation-by-operation complexity
// 1) append(int val)

// Work done: one modular subtraction, one modular multiply, and one modular inverse via modPow(a, M-2, M).
// Time: O(log⁡M)O(\log M)O(logM).
// Since M=1,000,000,007M = 1{,}000{,}000{,}007M=1,000,000,007, log⁡2M≈30\log_2 M \approx 30log2​M≈30, so this is effectively ~30 squarings/multiplies—fast but asymptotically O(log⁡M)O(\log M)O(logM).
// Space: amortized O(1)O(1)O(1) extra per call (push to vector). Over nnn appends, total O(n)O(n)O(n).


// Note: Each append recomputes the inverse of the current a. That is necessary because a may have changed due to multAll. There’s no cached inverse here, so it’s not O(1)O(1)O(1).

// 2) addAll(int inc)

// Work done: updates b = (b + inc) % mod.
// Time: O(1)O(1)O(1)
// Space: O(1)O(1)O(1)

// 3) multAll(int m)

// Work done: updates a = a * m % mod, b = b * m % mod.
// Time: O(1)O(1)O(1)
// Space: O(1)O(1)O(1)

// 4) getIndex(int idx)

// Work done: bounds check, then one modular multiply and add: (a * val[idx] + b) % mod.
// Time: O(1)O(1)O(1)
// Space: O(1)O(1)O(1)

// Overall complexities

// Per-operation

// append → O(log⁡M)O(\log M)O(logM)
// addAll → O(1)O(1)O(1)
// multAll → O(1)O(1)O(1)
// getIndex → O(1)O(1)O(1)

// For a sequence of qqq operations with nnn appends

// Time: O(nlog⁡M+(q−n))=O(nlog⁡M+q)O\big(n \log M + (q-n)\big) = O(n \log M + q)O(nlogM+(q−n))=O(nlogM+q)
// Space: O(n)O(n)O(n) for the stored normalized values.

// SOLUTION:

#include <iostream>
#include <vector>
using namespace std;

class Fancy {
private:
    const int mod = 1e9 + 7;
    vector<long long> val;
    long long a, b;
    long long modPow(long long x, long long y, long long mod) {
        long long res = 1;
        x = x % mod;
        while (y > 0) {
            if (y % 2 == 1) {
                res = (res * x) % mod;
            }
            y = y / 2;
            x = (x * x) % mod;
        }
        return res;
    }
public:
    Fancy() : a(1), b(0) {
    }
    
    void append(int val) {
        long long x = (val - b + mod) % mod;
        this->val.push_back((x * modPow(a, mod - 2, mod)) % mod);
    }
    
    void addAll(int inc) {
        b = (b + inc) % mod;
    }
    
    void multAll(int m) {
        a = (a * m) % mod;
        b = (b * m) % mod;
    }
    
    int getIndex(int idx) {
        if (idx >= val.size()) 
            return -1;
        return (a * val[idx] + b) % mod;
    }
};

/**
 * Your Fancy object will be instantiated and called as such:
 * Fancy* obj = new Fancy();
 * obj->append(val);
 * obj->addAll(inc);
 * obj->multAll(m);
 * int param_4 = obj->getIndex(idx);
 */