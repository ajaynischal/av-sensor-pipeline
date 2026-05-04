#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>

template <typename T, std::size_t Capacity>
class RingBuffer {
public:
  RingBuffer() = default;

  RingBuffer(const RingBuffer&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

  [[nodiscard]] bool push(T&& item) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (count_ >= Capacity) {
      return false;
    }
    storage_[write_idx_] = std::move(item);
    write_idx_ = (write_idx_ + 1U) % Capacity;
    ++count_;
    cv_.notify_one();
    return true;
  }

  [[nodiscard]] bool pop(T& item, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mtx_);
    const bool ready =
        cv_.wait_for(lock, timeout, [this] { return count_ > 0; });
    if (!ready) {
      return false;
    }
    item = std::move(storage_[read_idx_]);
    read_idx_ = (read_idx_ + 1U) % Capacity;
    --count_;
    return true;
  }

  [[nodiscard]] std::size_t size() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return count_;
  }

  [[nodiscard]] bool empty() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return count_ == 0;
  }

  [[nodiscard]] bool full() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return count_ >= Capacity;
  }

private:
  std::array<T, Capacity> storage_{};
  std::size_t read_idx_{0};
  std::size_t write_idx_{0};
  std::size_t count_{0};
  mutable std::mutex mtx_;
  std::condition_variable cv_;
};
