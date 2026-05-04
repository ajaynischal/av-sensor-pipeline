#include "sensor_producer.hpp"

#include <thread>

SensorProducer::SensorProducer(SensorType type, const std::string& sensor_id,
                               std::uint32_t frequency_hz,
                               RingBuffer<SensorMessage, 1024>& buffer)
    : type_(type),
      sensor_id_(sensor_id),
      frequency_hz_(frequency_hz),
      buffer_(buffer) {}

void SensorProducer::update_heartbeat() {
  last_heartbeat_us_.store(SensorMessage::now_timestamp_us(),
                           std::memory_order_relaxed);
}

void SensorProducer::run() {
  running_.store(true);
  const auto period = std::chrono::microseconds(
      1'000'000 / static_cast<std::int64_t>(frequency_hz_));

  while (running_.load(std::memory_order_relaxed)) {
    update_heartbeat();

    SensorMessage msg = generate_message();
    while (running_.load(std::memory_order_relaxed) &&
           !buffer_.push(std::move(msg))) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::this_thread::sleep_for(period);
  }
}

SensorMessage LidarProducer::generate_message() {
  SensorMessage m;
  m.type = SensorType::LIDAR;
  m.timestamp_us = SensorMessage::now_timestamp_us();
  m.sequence_id = sequence_++;
  m.sensor_id = sensor_id_;
  m.payload.resize(1024U);
  for (std::size_t i = 0; i < m.payload.size(); ++i) {
    m.payload[i] = static_cast<float>(i % 37);
  }
  return m;
}

SensorMessage CameraProducer::generate_message() {
  SensorMessage m;
  m.type = SensorType::CAMERA;
  m.timestamp_us = SensorMessage::now_timestamp_us();
  m.sequence_id = sequence_++;
  m.sensor_id = sensor_id_;
  m.payload.resize(256U);
  for (std::size_t i = 0; i < m.payload.size(); ++i) {
    m.payload[i] = static_cast<float>((i * 3U) % 101);
  }
  return m;
}

SensorMessage RadarProducer::generate_message() {
  SensorMessage m;
  m.type = SensorType::RADAR;
  m.timestamp_us = SensorMessage::now_timestamp_us();
  m.sequence_id = sequence_++;
  m.sensor_id = sensor_id_;
  m.payload.resize(64U);
  for (std::size_t i = 0; i < m.payload.size(); ++i) {
    m.payload[i] = static_cast<float>((i * 7U) % 53);
  }
  return m;
}
