#include "watchdog.hpp"

#include <thread>

Watchdog::Watchdog(const std::vector<SensorProducer*>& producers,
                   std::chrono::milliseconds timeout_ms,
                   std::chrono::milliseconds check_interval_ms)
    : producers_(producers),
      timeout_ms_(timeout_ms),
      check_interval_ms_(check_interval_ms) {}

Watchdog::~Watchdog() { stop(); }

void Watchdog::start() {
  stop_requested_.store(false);
  if (thread_.joinable()) {
    return;
  }
  thread_ = std::thread([this] { loop(); });
}

void Watchdog::stop() {
  stop_requested_.store(true);
  if (thread_.joinable()) {
    thread_.join();
  }
}

void Watchdog::loop() {
  while (!stop_requested_.load(std::memory_order_relaxed)) {
    const auto now_us = SensorMessage::now_timestamp_us();
    const auto timeout_us =
        std::chrono::duration_cast<std::chrono::microseconds>(timeout_ms_)
            .count();

    for (SensorProducer* prod : producers_) {
      if (prod == nullptr) {
        continue;
      }

      const std::uint64_t hb =
          prod->last_heartbeat_us_.load(std::memory_order_relaxed);
      if (hb == 0U) {
        continue;
      }

      const auto delta_us = static_cast<std::int64_t>(now_us - hb);
      const bool silent = delta_us > timeout_us;

      const std::string& sid = prod->sensor_id();
      const bool was_faulted = fault_state_[sid];

      if (silent && !was_faulted) {
        fault_state_[sid] = true;
        if (on_fault_detected) {
          on_fault_detected(sid);
        }
      } else if (!silent && was_faulted) {
        fault_state_[sid] = false;
        if (on_fault_recovered) {
          on_fault_recovered(sid);
        }
      }
    }

    std::this_thread::sleep_for(check_interval_ms_);
  }
}
