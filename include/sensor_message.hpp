#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

enum class SensorType : std::uint8_t { LIDAR, CAMERA, RADAR };

struct SensorMessage {
  SensorType type{};
  std::uint64_t timestamp_us{};
  std::uint32_t sequence_id{};
  std::vector<float> payload;
  std::string sensor_id;

  static std::uint64_t now_timestamp_us() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
  }
};
