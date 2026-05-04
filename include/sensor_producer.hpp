#pragma once

#include "ring_buffer.hpp"
#include "sensor_message.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

class SensorProducer {
public:
  SensorProducer(SensorType type, const std::string& sensor_id,
                 std::uint32_t frequency_hz,
                 RingBuffer<SensorMessage, 1024>& buffer);

  SensorProducer(const SensorProducer&) = delete;
  SensorProducer& operator=(const SensorProducer&) = delete;

  virtual ~SensorProducer() = default;

  virtual SensorMessage generate_message() = 0;

  void run();

  void stop() { running_.store(false); }

  [[nodiscard]] SensorType sensor_type() const noexcept { return type_; }

  [[nodiscard]] const std::string& sensor_id() const noexcept {
    return sensor_id_;
  }

  std::atomic<bool> running_{false};
  std::atomic<std::uint64_t> last_heartbeat_us_{0};

protected:
  SensorType type_;
  std::string sensor_id_;
  std::uint32_t frequency_hz_;
  RingBuffer<SensorMessage, 1024>& buffer_;

private:
  void update_heartbeat();
};

class LidarProducer final : public SensorProducer {
public:
  using SensorProducer::SensorProducer;

  SensorMessage generate_message() override;

private:
  std::uint32_t sequence_{0};
};

class CameraProducer final : public SensorProducer {
public:
  using SensorProducer::SensorProducer;

  SensorMessage generate_message() override;

private:
  std::uint32_t sequence_{0};
};

class RadarProducer final : public SensorProducer {
public:
  using SensorProducer::SensorProducer;

  SensorMessage generate_message() override;

private:
  std::uint32_t sequence_{0};
};
