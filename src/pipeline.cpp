#include "pipeline.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

Pipeline::Pipeline()
    : lidar_(std::make_unique<LidarProducer>(
          SensorType::LIDAR, "lidar-front", 10U, buffer_)),
      camera_(std::make_unique<CameraProducer>(
          SensorType::CAMERA, "camera-center", 30U, buffer_)),
      radar_(std::make_unique<RadarProducer>(
          SensorType::RADAR, "radar-long", 20U, buffer_)) {}

Pipeline::~Pipeline() { stop(); }

void Pipeline::start() {
  bool expected = false;
  if (!started_.compare_exchange_strong(expected, true)) {
    return;
  }

  consumer_running_.store(true);

  lidar_thread_ = std::thread([this] { lidar_->run(); });
  camera_thread_ = std::thread([this] { camera_->run(); });
  radar_thread_ = std::thread([this] { radar_->run(); });

  consumer_thread_ = std::thread([this] { consumer_loop(); });

  const std::vector<SensorProducer*> producer_ptrs{
      lidar_.get(),
      camera_.get(),
      radar_.get(),
  };

  watchdog_ = std::make_shared<Watchdog>(producer_ptrs);

  watchdog_->on_fault_detected = [this](const std::string& sid) {
    std::lock_guard<std::mutex> lock(log_mtx_);
    std::cout << "[WATCHDOG] FAULT: producer silent sensor_id=" << sid
              << std::endl;
  };

  watchdog_->on_fault_recovered = [this](const std::string& sid) {
    std::lock_guard<std::mutex> lock(log_mtx_);
    std::cout << "[WATCHDOG] RECOVERED: sensor_id=" << sid << std::endl;
  };

  watchdog_->start();
}

void Pipeline::stop() {
  bool expected = true;
  if (!started_.compare_exchange_strong(expected, false)) {
    return;
  }

  if (watchdog_) {
    watchdog_->stop();
    watchdog_.reset();
  }

  consumer_running_.store(false);
  if (consumer_thread_.joinable()) {
    consumer_thread_.join();
  }

  lidar_->stop();
  camera_->stop();
  radar_->stop();

  if (lidar_thread_.joinable()) {
    lidar_thread_.join();
  }
  if (camera_thread_.joinable()) {
    camera_thread_.join();
  }
  if (radar_thread_.joinable()) {
    radar_thread_.join();
  }
}

void Pipeline::consumer_loop() {
  while (consumer_running_.load(std::memory_order_relaxed)) {
    SensorMessage msg;
    if (!buffer_.pop(msg, std::chrono::milliseconds(100))) {
      std::lock_guard<std::mutex> lock(log_mtx_);
      std::cout << "[CONSUMER] buffer empty, waiting..." << std::endl;
      continue;
    }

    const auto now_us = SensorMessage::now_timestamp_us();
    const auto latency_us =
        static_cast<std::uint64_t>(now_us - msg.timestamp_us);

    msg_count_[msg.type] += 1U;
    latency_total_[msg.type] += latency_us;
    std::uint64_t& max_lat = latency_max_[msg.type];
    if (latency_us > max_lat) {
      max_lat = latency_us;
    }

    std::lock_guard<std::mutex> lock(log_mtx_);
    std::cout << '[' << msg.timestamp_us << "] [" << sensor_type_string(msg.type)
              << "] id=" << msg.sensor_id << " seq=" << msg.sequence_id
              << " payload_size=" << msg.payload.size()
              << " latency_us=" << latency_us << std::endl;
  }
}

const char* Pipeline::sensor_type_string(SensorType t) {
  switch (t) {
    case SensorType::LIDAR:
      return "LIDAR";
    case SensorType::CAMERA:
      return "CAMERA";
    case SensorType::RADAR:
      return "RADAR";
  }
  return "UNKNOWN";
}

void Pipeline::print_final_stats() const {
  auto count_for = [this](SensorType t) -> std::uint64_t {
    const auto it = msg_count_.find(t);
    return it == msg_count_.end() ? 0U : it->second;
  };
  auto total_for = [this](SensorType t) -> std::uint64_t {
    const auto it = latency_total_.find(t);
    return it == latency_total_.end() ? 0U : it->second;
  };
  auto max_for = [this](SensorType t) -> std::uint64_t {
    const auto it = latency_max_.find(t);
    return it == latency_max_.end() ? 0U : it->second;
  };

  const auto print_line = [&](const char* label, SensorType t) {
    const std::uint64_t cnt = count_for(t);
    const std::uint64_t tot = total_for(t);
    const std::uint64_t mx = max_for(t);
    const std::uint64_t avg = cnt > 0U ? tot / cnt : 0U;

    std::cout << std::left << std::setw(9) << label << std::right << std::setw(5)
              << cnt << std::left << " msgs  |  avg latency: " << std::right
              << std::setw(4) << avg << std::left << "us  |  max latency: "
              << std::right << std::setw(4) << mx << std::left << "us\n";
  };

  std::cout << "\n=== Final statistics (messages processed) ===\n";
  print_line("LIDAR:", SensorType::LIDAR);
  print_line("CAMERA:", SensorType::CAMERA);
  print_line("RADAR:", SensorType::RADAR);
}
