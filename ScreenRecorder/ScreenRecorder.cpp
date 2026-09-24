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

//головний клас

class ConfigManager {
private:
    int fps;
    int buffer_size_sec;
    std::string target_game;
public:
    ConfigManager() : fps(60), buffer_size_sec(30), target_game("Unknown") {}

    void loadDefaults() {
        fps = 60;
        buffer_size_sec = 15;
        target_game = "Forza Horizon 6";
    }

    int getFps() const { return fps; }
    int getBufferSizeFrames() const { return fps * buffer_size_sec; }
    std::string getTargetGame() const { return target_game; }
};

class Application {
private:
    std::shared_ptr<ILogger> logger;
    ConfigManager config;
    RingBuffer<MediaPacket> video_buffer;
    std::vector<std::unique_ptr<HighlightTrigger>> triggers;
    std::unique_ptr<MediaSource> video_source;
    bool is_running;

public:
    Application(std::shared_ptr<ILogger> log)
        : logger(log), video_buffer(600), is_running(false) {
    }

    void initialize() {
        try {
            logger->logInfo("Initializing Application...");
            config.loadDefaults();
            video_buffer.resize(config.getBufferSizeFrames());
            video_source = std::make_unique<MockVideoCapturer>(config.getTargetGame());
            triggers.push_back(std::make_unique<HotkeyTrigger>("Alt+F10"));
            triggers.push_back(std::make_unique<AudioVolumeTrigger>(100.0f));
            triggers.push_back(std::make_unique<TimerTrigger>(50));

            logger->logInfo("Hooked into target: " + config.getTargetGame());
        }
        catch (const std::exception& e) {
            logger->logError(std::string("Init failed: ") + e.what());
        }
    }

    void run() {
        is_running = true;
        video_source->start();
        logger->logInfo("Recording started. Waiting for highlights...");

        int loop_count = 0;
        while (is_running && loop_count < 100) {
            MediaPacket frame = video_source->grabNextPacket();
            video_buffer.push(frame);

            for (const auto& trigger : triggers) {
                if (trigger->checkCondition()) {
                    saveHighlight(trigger->getName());
                    video_buffer.clear();
                    break;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            loop_count++;
        }

        video_source->stop();
        logger->logInfo("Application shutdown safely.");
    }

    void saveHighlight(const std::string& reason) {
        auto frames = video_buffer.getAll();
        logger->logWarning("HIGHLIGHT SAVED! Trigger: " + reason);
        logger->logInfo("Encoded " + std::to_string(frames.size()) + " frames into highlight.mp4\n");
    }
};

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    std::shared_ptr<ILogger> consoleLog = std::make_shared<ConsoleLogger>();
    Application app(consoleLog);

    app.initialize();
    app.run();

    return 0;
}