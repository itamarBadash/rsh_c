#ifndef BASE_EVENTMANAGER_H
#define BASE_EVENTMANAGER_H

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <iostream>
#include <list>
#include <any>
#include <unordered_map>

template<typename... Args>
class Event {
public:
    using EventCallback = std::function<void(Args...)>;

    void subscribe(EventCallback callback) {
        callbacks.push_back(callback);
    }

    void unsubscribe(EventCallback callback) {
        callbacks.remove_if([&](const EventCallback& other) {
            return callback.target_type() == other.target_type();
        });
    }

    void invoke(Args... args) {
        for (auto& callback : callbacks) {
            callback(args...);
        }
    }

    void clear() {
        callbacks.clear();
    }

private:
    std::list<EventCallback> callbacks;
};

class EventManager {
public:
    template<typename... Args>
    void createEvent(const std::string& eventName) {
        std::lock_guard<std::mutex> lock(mutex);
        events[eventName] = std::make_any<Event<Args...>>();
        clearFunctions[eventName] = [this, eventName]() {
            std::any_cast<Event<Args...>&>(events[eventName]).clear();
        };
    }

    template<typename... Args>
    std::shared_ptr<Event<Args...>> getEvent(const std::string& eventName) {
        std::lock_guard<std::mutex> lock(mutex);
        return std::make_shared<Event<Args...>>(std::any_cast<Event<Args...>&>(events.at(eventName)));
    }

    template<typename... Args>
    void subscribe(const std::string& eventName, typename Event<Args...>::EventCallback callback) {
        getEvent<Args...>(eventName)->subscribe(callback);
    }

    template<typename... Args>
    void unsubscribe(const std::string& eventName, typename Event<Args...>::EventCallback callback) {
        getEvent<Args...>(eventName)->unsubscribe(callback);
    }

    template<typename... Args>
    void invoke(const std::string& eventName, Args... args) {
        getEvent<Args...>(eventName)->invoke(args...);
    }

    void removeEvent(const std::string& eventName) {
        std::lock_guard<std::mutex> lock(mutex);
        events.erase(eventName);
        clearFunctions.erase(eventName);
    }

    void clearEvent(const std::string& eventName) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = clearFunctions.find(eventName);
        if (it != clearFunctions.end()) {
            it->second();
        }
    }

    void clearAllEvents() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [name, clearFunc] : clearFunctions) {
            clearFunc();
        }
    }

    size_t eventCount() const {
        std::lock_guard<std::mutex> lock(mutex);
        return events.size();
    }

private:
    mutable std::mutex mutex;
    std::map<std::string, std::any> events;
    std::unordered_map<std::string, std::function<void()>> clearFunctions;
};

inline EventManager& GetEventManager() {
    static EventManager instance;
    return instance;
}

#endif // BASE_EVENTMANAGER_H
