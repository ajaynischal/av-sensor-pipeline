#include "sensor_producer.hpp"
#include "watchdog.hpp"

#include <cassert>
#include <chrono>
#include <thread>

namespace {

struct StubProducer final : SensorProducer {
  StubProducer(RingBuffer<SensorMessage, 1024>& buf)
      : SensorProducer(SensorType::LIDAR, "stub-lidar", 10U, buf) {}

  SensorMessage generate_message() override {
    SensorMessage m;
    m.type = SensorType::LIDAR;
    m.sensor_id = sensor_id_;
    return m;
  }
};

}  // namespace

int main() {
  RingBuffer<SensorMessage, 1024> buf;
  StubProducer stub(buf);
  stub.last_heartbeat_us_.store(SensorMessage::now_timestamp_us());

  std::atomic<int> faults{0};
  std::atomic<int> recoveries{0};

  Watchdog wd({&stub}, std::chrono::milliseconds(80),
              std::chrono::milliseconds(20));
  wd.on_fault_detected =
      [&](const std::string&) { faults.fetch_add(1, std::memory_order_relaxed); };
  wd.on_fault_recovered = [&](const std::string&) {
    recoveries.fetch_add(1, std::memory_order_relaxed);
  };

  wd.start();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  assert(faults.load(std::memory_order_relaxed) >= 1);

  stub.last_heartbeat_us_.store(SensorMessage::now_timestamp_us());
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  assert(recoveries.load(std::memory_order_relaxed) >= 1);

  wd.stop();
  return 0;
}
