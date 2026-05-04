#include "pipeline.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
  Pipeline pipeline;
  pipeline.start();

  std::this_thread::sleep_for(std::chrono::seconds(5));

  if (pipeline.lidar_producer() != nullptr) {
    pipeline.lidar_producer()->stop();
    std::cout << "[MAIN] Simulated fault: stopped lidar-front producer thread\n";
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));

  pipeline.stop();
  pipeline.print_final_stats();

  return 0;
}
