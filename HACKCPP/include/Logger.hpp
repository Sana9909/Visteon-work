#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <algorithm>

// ===== Template EventLogger =====
// Demonstrates: Templates, RAII, File I/O, Exception Handling, Lambdas, STL Algorithms
//
// Design decisions:
//   - Header-only template: enables instantiation with any streamable type T.
//   - RAII: the log file is opened in the constructor and closed in the destructor,
//     guaranteeing no resource leaks even if exceptions are thrown.
//   - Thread safety: every public method that touches shared state acquires logMutex_.
//   - Copy is deleted because std::ofstream is non-copyable and we want unique
//     ownership of the file handle (RAII semantics).
//   - Move is allowed so loggers can be transferred into containers or returned
//     from factory functions.
//   - logHistory_ keeps an in-memory mirror of all log entries so we can support
//     search and getRecentLogs without re-reading the file.
//   - logQueue_ provides a producer/consumer buffer for async logging: external
//     threads call enqueue(), and the logger thread calls processQueue().
template<typename T>
class EventLogger {
private:
    std::ofstream logFile_;
    std::string filePath_;
    mutable std::mutex logMutex_;
    std::vector<std::string> logHistory_;
    std::queue<std::string> logQueue_;
    bool isOpen_;

    // Helper: get current timestamp string
    // Uses platform-specific safe localtime conversion to avoid data races
    // that the C standard localtime() function is susceptible to.
    std::string getCurrentTimestamp() const {
        static std::time_t last_time = 0;
        static std::string last_time_str;
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        if (time == last_time && !last_time_str.empty()) {
            return last_time_str;
        }
        last_time = time;
        
        std::tm tm_buf;
        #ifdef _WIN32
            localtime_s(&tm_buf, &time);
        #else
            localtime_r(&time, &tm_buf);
        #endif
        
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        last_time_str = buf;
        return last_time_str;
    }

public:
    // RAII: Open file in constructor.
    // Throws std::runtime_error if the file cannot be opened, so callers
    // can catch and handle gracefully (e.g. fall back to stderr).
    explicit EventLogger(const std::string& filePath)
        : filePath_(filePath), isOpen_(false) {
        logFile_.open(filePath, std::ios::out | std::ios::trunc);
        if (!logFile_.is_open()) {
            throw std::runtime_error("EventLogger: Failed to open log file: " + filePath);
        }
        isOpen_ = true;
        // Write header to mark the start of this logging session
        logFile_ << "========================================" << std::endl;
        logFile_ << "  Vehicle Event Log Started" << std::endl;
        logFile_ << "  Timestamp: " << getCurrentTimestamp() << std::endl;
        logFile_ << "========================================" << std::endl;
        logFile_.flush();
    }

    // RAII: Close file in destructor.
    // Flushes any remaining queued messages before writing the footer and
    // closing the file, ensuring no data is lost.
    ~EventLogger() {
        if (isOpen_ && logFile_.is_open()) {
            // Flush remaining queue
            while (!logQueue_.empty()) {
                std::string entry = logQueue_.front();
                logQueue_.pop();
                logFile_ << entry << std::endl;
            }
            logFile_ << "========================================" << std::endl;
            logFile_ << "  Vehicle Event Log Ended" << std::endl;
            logFile_ << "  Timestamp: " << getCurrentTimestamp() << std::endl;
            logFile_ << "========================================" << std::endl;
            logFile_.flush();
            logFile_.close();
        }
    }

    // Delete copy (RAII — unique ownership of file handle)
    EventLogger(const EventLogger&) = delete;
    EventLogger& operator=(const EventLogger&) = delete;

    // Move is allowed so loggers can be transferred between owners
    EventLogger(EventLogger&& other) noexcept
        : logFile_(std::move(other.logFile_)),
          filePath_(std::move(other.filePath_)),
          logHistory_(std::move(other.logHistory_)),
          logQueue_(std::move(other.logQueue_)),
          isOpen_(other.isOpen_) {
        other.isOpen_ = false;
    }

    // Log an entry with category.
    // T must be streamable via operator<<. The entry is formatted with a
    // timestamp and category tag, written to the file immediately, and
    // appended to the in-memory history for later searching.
    void log(const T& entry, const std::string& category = "INFO") {
        std::lock_guard<std::mutex> lock(logMutex_);
        std::string logEntry;
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, const char*>) {
            logEntry = "[" + getCurrentTimestamp() + "] [" + category + "] " + std::string(entry);
        } else {
            std::ostringstream oss;
            oss << "[" << getCurrentTimestamp() << "] [" << category << "] " << entry;
            logEntry = oss.str();
        }
        
        if (isOpen_ && logFile_.is_open()) {
            logFile_ << logEntry << "\n";
        }
        logHistory_.push_back(std::move(logEntry));
    }

    // Convenience wrapper that logs with the "ALERT" category tag
    void logAlert(const T& entry) {
        log(entry, "ALERT");
    }

    // Enqueue a message for deferred writing.
    // This is the producer side of the async logging pattern: external
    // threads push messages here without blocking on file I/O.
    void enqueue(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex_);
        logQueue_.push(message);
    }

    // Process all queued messages (consumer side).
    // Called by the logger thread to drain the queue and write to disk.
    // Returns true if any messages were processed.
    bool processQueue() {
        std::lock_guard<std::mutex> lock(logMutex_);
        bool processed = false;
        while (!logQueue_.empty()) {
            std::string entry = logQueue_.front();
            logQueue_.pop();
            if (isOpen_ && logFile_.is_open()) {
                logFile_ << entry << std::endl;
            }
            logHistory_.push_back(entry);
            processed = true;
        }
        if (processed) {
            logFile_.flush();
        }
        return processed;
    }

    // Search logs using a lambda predicate — demonstrates STL algorithms.
    // Uses std::copy_if to filter logHistory_ entries matching the predicate.
    // Example: logger.search([](const std::string& s){ return s.find("ALERT") != std::string::npos; });
    std::vector<std::string> search(
            std::function<bool(const std::string&)> predicate) const {
        std::lock_guard<std::mutex> lock(logMutex_);
        std::vector<std::string> results;
        std::copy_if(logHistory_.begin(), logHistory_.end(),
                     std::back_inserter(results), predicate);
        return results;
    }

    // Get the most recent `count` log entries from history.
    // Useful for displaying a rolling window in the dashboard.
    std::vector<std::string> getRecentLogs(size_t count) const {
        std::lock_guard<std::mutex> lock(logMutex_);
        size_t start = logHistory_.size() > count 
                       ? logHistory_.size() - count : 0;
        return std::vector<std::string>(
            logHistory_.begin() + static_cast<long>(start),
            logHistory_.end());
    }

    // Query whether the underlying file is still open and writable
    bool isFileOpen() const { return isOpen_ && logFile_.is_open(); }

    // Return the total number of log entries recorded this session
    size_t getLogCount() const {
        std::lock_guard<std::mutex> lock(logMutex_);
        return logHistory_.size();
    }
};
