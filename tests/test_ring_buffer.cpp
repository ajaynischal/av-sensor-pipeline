#include "ring_buffer.hpp"

#include <cassert>
#include <chrono>
#include <thread>

int main() {
  RingBuffer<int, 4> rb;

  assert(rb.empty());
  assert(rb.size() == 0);
  assert(!rb.full());

  assert(rb.push(1));
  assert(rb.size() == 1);
  assert(!rb.empty());
  assert(!rb.full());

  assert(rb.push(2));
  assert(rb.push(3));
  assert(rb.push(4));
  assert(rb.full());
  assert(rb.size() == 4);
  assert(!rb.push(5));

  int v = 0;
  assert(rb.pop(v, std::chrono::milliseconds(100)));
  assert(v == 1);
  assert(rb.push(5));
  assert(rb.full());

  assert(rb.pop(v, std::chrono::milliseconds(100)));
  assert(rb.pop(v, std::chrono::milliseconds(100)));
  assert(rb.pop(v, std::chrono::milliseconds(100)));
  assert(rb.pop(v, std::chrono::milliseconds(100)));
  assert(rb.empty());

  assert(!rb.pop(v, std::chrono::milliseconds(30)));

  std::thread producer([&rb] {
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    assert(rb.push(42));
  });

  assert(rb.pop(v, std::chrono::milliseconds(200)));
  assert(v == 42);
  producer.join();

  return 0;
}
