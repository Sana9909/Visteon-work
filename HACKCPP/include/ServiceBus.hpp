#pragma once

#include <string>
#include <vector>
#include <functional>
#include <shared_mutex>
#include <map>
#include <memory>
#include <mutex>

// ----- Service Oriented Data Types -----

struct SensorEvent {
    std::string name;
    double value;
    std::string unit;
    std::string timestamp;
};

struct AlertEvent {
    std::string source;
    std::string message;
    std::string severity;
    bool isActive;
};

struct DtcEvent {
    std::string code;
    std::string description;
    bool isConfirmed;
};

// ----- Service Bus Class -----
// A thread-safe publisher-subscriber message bus demonstrating C++17 templated registry.
class ServiceBus {
public:
    template <typename EventType>
    using SubscriberCallback = std::function<void(const EventType&)>;

    // Get singleton instance
    static ServiceBus& getInstance();

    // Subscribe to a typed event topic
    template <typename EventType>
    void subscribe(const std::string& topic, SubscriberCallback<EventType> callback) {
        std::unique_lock<std::shared_mutex> lock(busMutex_);
        auto& list = getCallbackList<EventType>(topic);
        list.push_back(callback);
    }

    // Publish a typed event to all active subscribers
    template <typename EventType>
    void publish(const std::string& topic, const EventType& event) {
        std::shared_lock<std::shared_mutex> lock(busMutex_);
        auto it = getTopicRegistry<EventType>().find(topic);
        if (it != getTopicRegistry<EventType>().end()) {
            for (const auto& callback : it->second) {
                if (callback) {
                    callback(event);
                }
            }
        }
    }

private:
    ServiceBus() = default;
    ~ServiceBus() = default;
    ServiceBus(const ServiceBus&) = delete;
    ServiceBus& operator=(const ServiceBus&) = delete;

    std::shared_mutex busMutex_;

    // Helper: returns the static maps containing type-specific callbacks
    template <typename EventType>
    std::map<std::string, std::vector<SubscriberCallback<EventType>>>& getTopicRegistry() {
        static std::map<std::string, std::vector<SubscriberCallback<EventType>>> registry;
        return registry;
    }

    // Helper: returns topic-specific list of callbacks
    template <typename EventType>
    std::vector<SubscriberCallback<EventType>>& getCallbackList(const std::string& topic) {
        return getTopicRegistry<EventType>()[topic];
    }
};
