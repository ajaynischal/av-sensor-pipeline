# av-sensor-pipeline

This project is a small multi-threaded simulation where synthetic lidar, camera, and radar producers publish `SensorMessage` frames into a lock-based ring buffer while a consumer drains and logs them, with a watchdog that detects silent producers.

## Why this matters for autonomous vehicle software

Real AV stacks ingest high-rate sensor data under tight timing budgets; bounded queues, explicit concurrency control, and health monitoring prevent silent failures from starving downstream perception and planning. This demo mirrors those constraints at a portfolio-friendly scale.

## Architecture

```
LidarProducer  \
CameraProducer  +-->  RingBuffer<SensorMessage,1024>  -->  Consumer
RadarProducer  /                 |
                          Watchdog
                    (monitors all producers)
```

## Build

```bash
cmake -B build && cmake --build build
```

## Run

```bash
./build/av-sensor-pipeline
```

## Design decisions

- **Lock-based ring buffer with condition variables** — Gives predictable blocking semantics for the consumer (`pop` with timeout) while keeping `push` non-blocking so producers never deadlock when the queue is momentarily full.
- **`std::atomic` heartbeats + watchdog thread** — Producer health is observed lock-free on the hot path; the watchdog batches checks on its own cadence, matching production patterns that isolate fault detection from data-plane throughput.
- **RAII ownership (`unique_ptr` / `shared_ptr`) and ordered shutdown** — Every thread is joined, watchdog and producers stop in reverse launch order, and destructors guarantee cleanup without detached threads or manual `new`/`delete`.

## Sample output

Normal steady-state lines look like:

```
[1735123456789012] [RADAR] id=radar-long seq=401 payload_size=64 latency_us=812
[1735123456789456] [CAMERA] id=camera-center seq=902 payload_size=256 latency_us=633
[1735123456790111] [LIDAR] id=lidar-front seq=103 payload_size=1024 latency_us=905
[CONSUMER] buffer empty, waiting...
```

After the simulated lidar stall (`stop()` at five seconds), the watchdog raises a fault once the heartbeat age exceeds 500 ms:

```
[MAIN] Simulated fault: stopped lidar-front producer thread
[WATCHDOG] FAULT: producer silent sensor_id=lidar-front
```

When the demo ends, the consumer prints aggregate counts per modality:

```
=== Final statistics (messages processed) ===
LIDAR:  42
CAMERA: 287
RADAR:  191
```
