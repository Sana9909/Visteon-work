#include <iostream>
#include <functional>
#include <string>

// --- The Utility Class ---
class ScopeGuard {
private:
    std::function<void()> onExit;
    bool active; // Advanced addition: allows us to "dismiss" the guard if needed

public:
    // Explicit prevents accidental implicit conversions
    explicit ScopeGuard(std::function<void()> f) 
        : onExit(std::move(f)), active(true) {}

    // The Destructor: This is the heart of RAII. 
    // It runs no matter HOW the scope is exited.
    ~ScopeGuard() {
        if (active && onExit) {
            onExit();
        }
    }

    // DISALLOW COPYING: We don't want two guards running the same cleanup logic.
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    // ALLOW MOVING: So we can pass guards between functions if necessary.
    ScopeGuard(ScopeGuard&& other) noexcept 
        : onExit(std::move(other.onExit)), active(other.active) {
        other.active = false;
    }

    // Dismiss: Useful if the operation succeeds and you DON'T want cleanup (e.g., Commit)
    void dismiss() { active = false; }
};

// --- Mock Database Functions ---
void connect() { std::cout << "[System] Database Connected.\n"; }
void disconnect() { std::cout << "[System] Database Disconnected safely.\n"; }

// --- Simulation Function ---
void updateDatabase(bool simulateError) {
    connect();

    // Initialize the Guard. 
    // From this line forward, disconnect() IS GUARANTEED to run.
    ScopeGuard cleanup([]() {
        disconnect();
    });

    std::cout << "[Process] Modifying sensitive data...\n";

    if (simulateError) {
        std::cout << "[Process] ERROR ENCOUNTERED! Returning early...\n";
        return; // The 'cleanup' object's destructor is called HERE.
    }

    std::cout << "[Process] All operations successful.\n";
} // The 'cleanup' object's destructor is called HERE if no error occurred.

int main() {
    std::cout << "--- Scenario 1: Normal Execution ---\n";
    updateDatabase(false);

    std::cout << "\n--- Scenario 2: Error/Early Return ---\n";
    updateDatabase(true);

    return 0;
}


// Why this is Advanced C++
// 1. Handling the "Early Return" Problem
// In C or Java/Python (without with or try-finally), if you have five different return statements or an exception 
// is thrown, you have to remember to call disconnect() before every single one. In C++, the Stack Unwinding 
// mechanism ensures that when a function exits, all local objects are destroyed in reverse order of creation. 
// The ScopeGuard hitches a ride on that mechanism.

// 2. std::function vs. Function Pointers
// We use std::function<void()> so the guard can accept:

// Lambdas (as seen in the example).

// Function Pointers.

// Bind expressions.

// Functors (classes with operator()).

// 3. The "Dismiss" Feature (Transaction Pattern)
// In advanced database programming, you often use a "Commit" pattern. You set up a guard to Rollback changes if 
// something fails. If the function reaches the very end successfully, you call guard.dismiss(). 
// This tells the destructor "don't do the cleanup, the data is safe now."

// 4. The Move Constructor (noexcept)
// I added a Move Constructor to the code. By marking it noexcept, we tell the compiler it’s safe to move 
// this object without it throwing an exception, which is critical for performance when using these guards 
// in STL containers like std::vector.