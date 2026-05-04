#pragma once

#include "sensor_producer.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class Watchdog {
public:
  Watchdog(const std::vector<SensorProducer*>& producers,
           std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(500),
           std::chrono::milliseconds check_interval_ms =
               std::chrono::milliseconds(100));

  Watchdog(const Watchdog&) = delete;
  Watchdog& operator=(const Watchdog&) = delete;

  ~Watchdog();

  std::function<void(const std::string&)> on_fault_detected;
  std::function<void(const std::string&)> on_fault_recovered;

  void start();
  void stop();

private:
  void loop();

  std::vector<SensorProducer*> producers_;
  std::chrono::milliseconds timeout_ms_;
  std::chrono::milliseconds check_interval_ms_;
  std::unordered_map<std::string, bool> fault_state_;
  std::atomic<bool> stop_requested_{false};
  std::thread thread_;
};
