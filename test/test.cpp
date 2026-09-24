#include "pch.h"
#include <vector>
#include <mutex>
#include <string>

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

TEST(RingBufferTest, Initialization) {
    RingBuffer<int> buffer(5);
    EXPECT_EQ(buffer.getCapacity(), 5);
    EXPECT_EQ(buffer.getAll().size(), 0);
}

TEST(RingBufferTest, PushAndWrapAround) {
    RingBuffer<int> buffer(3);

    // Заповнюємо буфер
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    auto data = buffer.getAll();
    EXPECT_EQ(data.size(), 3);
    EXPECT_EQ(data[0], 1);
    EXPECT_EQ(data[2], 3);
    buffer.push(4);
    auto newData = buffer.getAll();

    EXPECT_EQ(newData.size(), 3);
    EXPECT_EQ(newData[0], 2); 
    EXPECT_EQ(newData[2], 4); 
}

TEST(RingBufferTest, ClearBuffer) {
    RingBuffer<int> buffer(5);
    buffer.push(10);
    buffer.push(20);
    buffer.clear();

    EXPECT_EQ(buffer.getAll().size(), 0);
}

TEST(RingBufferTest, ResizeBuffer) {
    RingBuffer<int> buffer(2);
    buffer.push(10);
    buffer.resize(10);

    EXPECT_EQ(buffer.getCapacity(), 10);
    EXPECT_EQ(buffer.getAll().size(), 0); 
}