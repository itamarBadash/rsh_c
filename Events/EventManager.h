#ifndef BASE_EVENTMANAGER_H
#define BASE_EVENTMANAGER_H

#pragma once

#include <functional>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <algorithm>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

template<typename... Args>
class Event {
public:
    using EventCallback = std::function<void(Args...)>;

    void subscribe(EventCallback callback) {
        std::lock_guard<std::mutex> lock(mutex);
        callbacks.push_back(callback);
    }

    void unsubscribe(EventCallback callback) {
        std::lock_guard<std::mutex> lock(mutex);
        callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
                                       [&callback](const EventCallback& cb) {
                                           return cb.target_type() == callback.target_type();
                                       }), callbacks.end());
    }

    void invoke(Args... args) {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& callback : callbacks) {
            callback(args...);
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        callbacks.clear();
    }

    size_t subscriberCount() const {
        std::lock_guard<std::mutex> lock(mutex);
        return callbacks.size();
    }

private:
    mutable std::mutex mutex;
    std::vector<EventCallback> callbacks;
};

class EventManager {
public:
    template<typename... Args>
    using EventPtr = std::shared_ptr<Event<Args...>>;

    template<typename... Args>
    EventPtr<Args...> getEvent(const std::string& eventName) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = events.find(eventName);
        if (it == events.end()) {
            auto event = std::make_shared<Event<Args...>>();
            events[eventName] = event;
            eventTypes[eventName] = std::type_index(typeid(Event<Args...>));
            return event;
        }
        auto event = std::dynamic_pointer_cast<Event<Args...>>(it->second);
        if (!event) {
            throw std::runtime_error("Event exists with different signature");
        }
        return event;
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
        eventTypes.erase(eventName);
    }

    void clearEvent(const std::string& eventName) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = events.find(eventName);
        if (it != events.end()) {
            auto eventType = eventTypes[eventName];
            if (eventType == std::type_index(typeid(Event<void>))) {
                std::dynamic_pointer_cast<Event<void>>(it->second)->clear();
            }
            // Add other types here if necessary
        }
    }

    void clearAllEvents() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [name, event] : events) {
            auto eventType = eventTypes[name];
            if (eventType == std::type_index(typeid(Event<void>))) {
                std::dynamic_pointer_cast<Event<void>>(event)->clear();
            }
            // Add other types here if necessary
        }
    }

    size_t eventCount() const {
        std::lock_guard<std::mutex> lock(mutex);
        return events.size();
    }

private:
    mutable std::mutex mutex;
    std::map<std::string, std::shared_ptr<void>> events;
    std::unordered_map<std::string, std::type_index> eventTypes;
};

inline EventManager& GetEventManager() {
    static EventManager instance;
    return instance;
}

// Macros for easier use
#define SUBSCRIBE_EVENT(eventName, ...) GetEventManager().subscribe(eventName, __VA_ARGS__)
#define UNSUBSCRIBE_EVENT(eventName, ...) GetEventManager().unsubscribe(eventName, __VA_ARGS__)
#define INVOKE_EVENT(eventName, ...) GetEventManager().invoke(eventName, __VA_ARGS__)
#define REMOVE_EVENT(eventName) GetEventManager().removeEvent(eventName)
#define CLEAR_EVENT(eventName) GetEventManager().clearEvent(eventName)
#define CLEAR_ALL_EVENTS() GetEventManager().clearAllEvents()

#endif //BASE_EVENTMANAGER_H
