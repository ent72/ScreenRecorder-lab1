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
// тригери

class HighlightTrigger {
protected:
    std::string name;
public:
    explicit HighlightTrigger(std::string triggerName) : name(std::move(triggerName)) {}
    virtual ~HighlightTrigger() = default;
    virtual bool checkCondition() = 0;
    std::string getName() const { return name; }
};

class HotkeyTrigger : public HighlightTrigger {
private:
    std::string key_combo;
public:
    HotkeyTrigger(std::string key) : HighlightTrigger("Hotkey"), key_combo(std::move(key)) {}

    bool checkCondition() override {
        return (rand() % 100) > 95;
    }
};

class AudioVolumeTrigger : public HighlightTrigger {
private:
    float threshold_db;
    float current_mock_volume;
public:
    AudioVolumeTrigger(float threshold) : HighlightTrigger("AudioSpike"), threshold_db(threshold), current_mock_volume(0.0f) {}

    bool checkCondition() override {
        current_mock_volume = static_cast<float>(rand() % 120);
        return current_mock_volume > threshold_db;
    }
};

class TimerTrigger : public HighlightTrigger {
private:
    int interval_seconds;
    int ticks;
public:
    TimerTrigger(int seconds) : HighlightTrigger("Timer"), interval_seconds(seconds), ticks(0) {}

    bool checkCondition() override {
        ticks++;
        if (ticks >= interval_seconds) {
            ticks = 0;
            return true;
        }
        return false;
    }
};

//захоплення зображення

class MediaSource {
protected:
    std::string source_name;
    bool is_capturing = false;
public:
    MediaSource(std::string name) : source_name(std::move(name)) {}
    virtual ~MediaSource() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual MediaPacket grabNextPacket() = 0;
};

class MockVideoCapturer : public MediaSource {
private:
    int frame_counter = 0;
    std::string target_app;
public:
    MockVideoCapturer(std::string app) : MediaSource("DesktopDuplication"), target_app(std::move(app)) {}

    void start() override {
        is_capturing = true;
    }

    void stop() override {
        is_capturing = false;
    }

    MediaPacket grabNextPacket() override {
        frame_counter++;
        return { frame_counter, 0, "PixelData_From_" + target_app };
    }
};

int main() {
    return 0;
}