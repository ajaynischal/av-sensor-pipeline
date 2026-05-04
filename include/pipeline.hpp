#pragma once

#include "ring_buffer.hpp"
#include "sensor_message.hpp"
#include "sensor_producer.hpp"
#include "watchdog.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace std {
template <>
struct hash<SensorType> {
  std::size_t operator()(SensorType t) const noexcept {
    return static_cast<std::size_t>(t);
  }
};
}  // namespace std

class Pipeline {
public:
  Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;

  ~Pipeline();

  void start();
  void stop();

  [[nodiscard]] LidarProducer* lidar_producer() noexcept {
    return lidar_.get();
  }

  void print_final_stats() const;

private:
  void consumer_loop();
  static const char* sensor_type_string(SensorType t);

  RingBuffer<SensorMessage, 1024> buffer_;
  std::unique_ptr<LidarProducer> lidar_;
  std::unique_ptr<CameraProducer> camera_;
  std::unique_ptr<RadarProducer> radar_;
  std::shared_ptr<Watchdog> watchdog_;

  std::mutex log_mtx_;

  std::thread lidar_thread_;
  std::thread camera_thread_;
  std::thread radar_thread_;
  std::thread consumer_thread_;

  std::atomic<bool> consumer_running_{false};
  std::atomic<bool> started_{false};

  std::unordered_map<SensorType, std::uint64_t> latency_total_;
  std::unordered_map<SensorType, std::uint64_t> latency_max_;
  std::unordered_map<SensorType, std::uint64_t> msg_count_;
};
