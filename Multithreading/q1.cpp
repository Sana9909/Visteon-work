// Q1. You have a function that creates a large Database connection object and needs
// to pass it to a Background Worker thread, but the original function will finish immediately.
// Which smart pointer would you use?
// How would you pass it to the thread to ensure the memory stays alive exactly as long as the worker needs it?

// SOLUTION:
// To implement this correctly, we use std::thread and std::move. The "advanced" part here is
// understanding that std::move on a shared_ptr transfers the ownership to the thread's internal storage without 
// incrementing the reference count, which is a common performance optimization.

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <string>

// A mock Database Connection resource
class DbConnection {
public:
    DbConnection(std::string id) : connectionId(id) {
        std::cout << "[DB] Connection " << connectionId << " opened.\n";
    }
    ~DbConnection() {
        std::cout << "[DB] Connection " << connectionId << " closed.\n";
    }
    void executeQuery(const std::string& query) const {
        std::cout << "[DB] Executing: " << query << " on " << connectionId << "\n";
    }

private:
    std::string connectionId;
};

// The Worker Function (runs on a background thread)
void backgroundWorker(std::shared_ptr<DbConnection> db) {
    std::cout << "[Worker] Background task started...\n";
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    db->executeQuery("SELECT * FROM users;");
    
    std::cout << "[Worker] Background task finished.\n";
} // 'db' goes out of scope here. If it's the last owner, the connection closes.

int main() {
    // 1. Create the connection using make_shared (Best Practice)
    auto connection = std::make_shared<DbConnection>("PROD_DB_01");

    std::cout << "[Main] Reference count before move: " << connection.use_count() << "\n";

    // 2. Start the thread and MOVE the shared_ptr into it
    // After std::move, 'connection' in main() becomes nullptr.
    std::thread t(backgroundWorker, std::move(connection));

    // Notice: connection is now null, but the object lives on in the thread
    if (!connection) {
        std::cout << "[Main] Connection ownership transferred. main's pointer is null.\n";
    }

    // 3. Main finishes its other tasks and detaches or joins
    std::cout << "[Main] Doing other things...\n";
    
    t.join(); // Wait for the worker to finish

    std::cout << "[Main] Program exiting.\n";
    return 0;
}

//EXPLANATION:
// - We create a shared_ptr to manage the DbConnection resource.
// - We use std::move to transfer ownership of the shared_ptr to the background thread.
// - The background thread will keep the connection alive until it finishes its work, at which point the destructor will be called if it's the last owner.
// Why this is the "Advanced" way:
// Ownership Transfer via std::move: By moving the shared_ptr into the thread constructor, we avoid the atomic 
// increment/decrement of the reference count. The thread's internal storage takes the pointer directly from main.
// Thread Safety of Ref-Counting: Even if multiple threads were sharing this DbConnection, the shared_ptr internal 
// control block uses atomic operations to ensure the reference count is updated safely across CPU cores.
// Guaranteed Cleanup: If main finishes or throws an exception, the DbConnection remains alive because the thread 
// holds an owner. The moment the thread finishes and its local db variable is destroyed, the connection is closed instantly.
