#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>

//базові структури

struct MediaPacket {
    int id;
    long long timestamp_ms;
    std::string mock_data;
};

template <typename T>
class RingBuffer {
private:
    std::vector<T> buffer;
    size_t head = 0;
    size_t tail = 0;
    size_t max_size;
    bool full = false;
    std::mutex mtx;

public:
    explicit RingBuffer(size_t size) : max_size(size), buffer(size) {}
    void push(T item) {
        std::lock_guard<std::mutex> lock(mtx);
        buffer[head] = item;
        if (full) {
            tail = (tail + 1) % max_size;
        }
        head = (head + 1) % max_size;
        full = head == tail;
    }

    std::vector<T> getAll() {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<T> result;
        size_t current = tail;
        size_t count = full ? max_size : head;
        for (size_t i = 0; i < count; ++i) {
            result.push_back(buffer[current]);
            current = (current + 1) % max_size;
        }
        return result;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx);
        head = tail = 0;
        full = false;
    }

    void resize(size_t new_size) {
        std::lock_guard<std::mutex> lock(mtx);
        buffer.resize(new_size);
        max_size = new_size;
        head = tail = 0;
        full = false;
    }

    size_t getCapacity() const { return max_size; }
};

// логування

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void logInfo(const std::string& message) = 0;
    virtual void logWarning(const std::string& message) = 0;
    virtual void logError(const std::string& message) = 0;
};

class ConsoleLogger : public ILogger {
public:
    void logInfo(const std::string& message) override {
        std::cout << "[INFO] " << message << "\n";
    }
    void logWarning(const std::string& message) override {
        std::cout << "[WARN] " << message << "\n";
    }
    void logError(const std::string& message) override {
        std::cerr << "[ERROR] " << message << "\n";
    }
};

int main() {
    return 0;
}