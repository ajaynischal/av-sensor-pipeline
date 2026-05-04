# av-sensor-pipeline

A multi-threaded C++17 sensor data pipeline simulating the ingestion layer
of an autonomous vehicle software stack.

## What it does

Three concurrent sensor producer threads (lidar at 10hz, camera at 30hz,
radar at 20hz) push typed messages into a thread-safe ring buffer. A
consumer thread reads and processes messages in real time, logging
per-message latency. A watchdog timer monitors all producers and raises
a fault alert within 500ms if any producer goes silent.

## Why this is relevant to autonomous vehicle software

On-vehicle AV software runs multiple sensor pipelines concurrently with
strict latency and fault tolerance requirements. This project implements
the core patterns used in that context: lock-based concurrent queuing,
atomic heartbeat monitoring, RAII resource management, and structured
fault detection with recovery callbacks. The architecture mirrors a
real sensor ingestion layer where lidar, camera, and radar drivers feed
a central processing pipeline.

## Architecture
LidarProducer  (10hz)  
CameraProducer (30hz)  +-->  RingBuffer<SensorMessage, 1024>  -->  Consumer
RadarProducer  (20hz)  /| Watchdog (polls every 100ms, faults at 500ms)

## Prerequisites

CMake >= 3.16
C++17 compiler (clang++ or g++)
pthreads (standard on Linux and macOS)

## Build

```bash
cmake -B build && cmake --build build
```

## Run

```bash
./build/av-sensor-pipeline
```

## Run tests

```bash
./build/test_ring_buffer
./build/test_watchdog
```

## Sample output
[1777871404290350] [CAMERA] id=camera-center seq=264 payload_size=256 latency_us=17
[1777871404310151] [RADAR]  id=radar-long    seq=183 payload_size=64  latency_us=7
[WATCHDOG] FAULT detected: lidar-front (no heartbeat for 500ms)
[1777871404364621] [RADAR]  id=radar-long    seq=184 payload_size=64  latency_us=8
...
=== Final statistics (messages processed) ===
LIDAR:   49 msgs  |  avg latency:  12us  |  max latency:  45us
CAMERA: 272 msgs  |  avg latency:   9us  |  max latency:  38us
RADAR:  189 msgs  |  avg latency:  11us  |  max latency:  37us

## Key design decisions

- **RingBuffer uses a mutex and condition variable, not a spin lock.**
  A spin lock wastes CPU cycles polling continuously. A condition variable
  puts the consumer thread to sleep and wakes it only when data arrives,
  which is correct behaviour for a pipeline where producers run at fixed
  frequencies with idle gaps between messages.

- **Heartbeat is std::atomic not a mutex-protected value.**
  The watchdog reads last_heartbeat_us_ while the producer writes it
  from a different thread. Making it atomic avoids a data race without
  the overhead of locking, which matters in a high-frequency loop.

- **All threads are joined in destructors, never detached.**
  A detached thread that outlives its data causes undefined behaviour.
  RAII join in each destructor guarantees every thread finishes before
  the owning object is destroyed, making shutdown deterministic and safe.