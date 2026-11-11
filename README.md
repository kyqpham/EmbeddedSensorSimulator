# Embedded Sensor Simulator

A C++17 project that simulates the software side of an embedded sensor system. One shared sensor framework (`sensor.h`) is used by two programs:

1. **`main.cpp`**: a single-threaded demo of the framework. Fake temperature, pressure, and IMU sensors share an abstract interface, are created through a factory, and each logs its recent readings into a hand-written fixed-size **ring buffer**.
2. **`pipeline.cpp`**: a multithreaded **producer/consumer telemetry pipeline** built on the same sensors. Each sensor runs on its own thread and pushes readings into a shared queue. A consumer thread processes them, flags out-of-range values, and reports through a **thread-safe singleton logger**.

`threads.cpp` is a standalone set of concurrency experiments (a reproducible race condition and its fix) that led to the pipeline's design.

## Repository layout

| File | What it contains |
|---|---|
| `sensor.h` | The shared framework: `RingBuffer`, abstract `Sensor`, three concrete sensors, and the `Sensor::create` factory |
| `main.cpp` | Single-threaded demo: factory, static instance count, each sensor logging into its own ring buffer |
| `pipeline.cpp` | Multithreaded pipeline: one producer thread per sensor, a shared queue, a consumer with fault detection, and a singleton logger |
| `threads.cpp` | Standalone experiments: `std::thread`, `join()`, a lost-update race condition, and the mutex fix |

## Build and run

```bash
# sensor framework demo
clang++ -std=c++17 -Wall -Wextra -g main.cpp -o sim && ./sim

# race condition demo (run it many times; races are intermittent)
clang++ -std=c++17 -Wall -Wextra -g -pthread threads.cpp -o threads
for i in {1..10}; do ./threads; done

# producer/consumer pipeline
clang++ -std=c++17 -Wall -Wextra -g -pthread pipeline.cpp -o pipeline && ./pipeline
```

`-pthread` enables thread support. It's optional with clang on macOS but required with `g++` on Linux.

---

# Part 1: Sensor framework (`sensor.h`, demo in `main.cpp`)

Example output of `main.cpp`:

```
count: 0
thermometer: 21.5 22 22.5 23
pressure: 101.3 101.3 101.3 101.3
imu: 9.8 9.8 9.8 9.8
count: 3
```

followed by each sensor's destructor messages (derived first, then `Sensor`). The thermometer took 6 readings (20.5 → 23.0), but its buffer holds only 4, so the two oldest were overwritten.

## Structure

| Class / function | Role |
|---|---|
| `RingBuffer` | Fixed-capacity circular buffer of readings (capacity 4) |
| `Sensor` | Abstract base class: the contract every sensor must fulfill |
| `Thermometer`, `PressureSensor`, `IMUSensor` | Concrete sensors returning fake data |
| `Sensor::create(type)` | Static factory: builds a sensor from a text name |
| `report(Sensor&)` | Free function that works on *any* sensor |

## Concepts

**Abstract classes.** `Sensor` declares `read()` and `name()` as pure virtual (`= 0`), so it can't be instantiated. It's a contract: every sensor must be able to read and name itself. A default like `return 0.0;` would hide bugs. Pure virtual turns a forgotten implementation into a compile error.

**Inheritance.** `class Thermometer : public Sensor` means *a Thermometer **is a** Sensor*. Subclasses inherit the shared parts (`category()`, `sample()`, `printLog()`, the log buffer, the instance counter) and supply their own `read()` and `name()`.

**Virtual functions and polymorphism.** Because `read()` is virtual, a call through a `Sensor&` or `Sensor*` is resolved at runtime by the object's real type. `report()` and the pipeline's producer function are each written once and work for every sensor. `Sensor::sample()` is non-virtual but calls the virtual `read()`, so each sensor logs its own data.
*Experiment (during development):* removing `virtual` made a function taking `Sensor&` print the base version's `0` instead of the thermometer's reading, because the call was bound at compile time to the declared type.

**`override`.** Every overriding function is marked `override`, so the compiler verifies a matching virtual function exists. It caught a `const` mismatch between `Sensor::read()` and `Thermometer::read()` during development.

**Virtual destructors.** Sensors are owned through base-class pointers, so `~Sensor()` is virtual. Destruction then runs the derived destructor first, then the base one.
*Experiment:* with a non-virtual destructor, deleting through a `Sensor*` skipped `~Thermometer()` entirely, and clang warned at compile time. That's undefined behavior, and a leak if the derived class owns resources.

**References vs. copies.** Passing a sensor by reference or pointer works on the caller's real object. Passing by value would **slice** a `Thermometer` down to a plain `Sensor`.

**Smart pointers.** Sensors live in a `std::vector<std::unique_ptr<Sensor>>`. `unique_ptr` owns its object and deletes it automatically (no manual `delete`, no leaks). It's move-only, so the loop iterates by reference: `for (const auto& s : sensors)`.

**Header file.** The framework lives in `sensor.h` so both programs can `#include` it. `#pragma once` prevents it from being included twice, and `Sensor::create` is marked `inline` because a function defined in a header may be compiled into more than one file.

**Statics.**
- `static int count`: one counter shared by the class. The base constructor and destructor update it, so it tracks live sensors of every type.
- `Sensor::create("temp")`: a static factory that needs no existing object and returns `nullptr` for unknown types.
- `static constexpr int capacity`: a compile-time constant shared by every ring buffer.

**Composition.** Each `Sensor` **has a** private `RingBuffer`. Inheritance models *is-a*, and composition models *has-a*. Separate buffers keep each sensor's history labeled and stop a fast sensor from overwriting a slow one's data.

**`const` and encapsulation.** Read-only methods are `const`. `read()` deliberately isn't, because the thermometer updates internal state on each read. `RingBuffer`'s `data`, `head`, and `count` are private, so only its own methods can change the bookkeeping.

## The ring buffer

A fixed array used as a circle. When full, a new reading **overwrites the oldest**, so memory use is fixed forever and the most recent history is always available. That's ideal for embedded systems.

- `head`: index of the oldest reading
- `count`: number of stored readings
- next write slot (not full): `(head + count) % capacity`
- i-th oldest reading: `(head + i) % capacity`

```
push 10,20,30,40  -> [10 20 30 40]  head=0 count=4   (full)
push 50           -> [50 20 30 40]  head=1 count=4   (overwrote 10)
push 60           -> [50 60 30 40]  head=2 count=4   (overwrote 20)
oldest -> newest:     30 40 50 60
```

---

# Part 2: Threads and race conditions (`threads.cpp`)

**Threads.** `std::thread t(func, args...)` starts `func` running concurrently. `t.join()` blocks until it finishes. Every thread must be joined before its `std::thread` object is destroyed, and results are only safe to read after `join()`.

**Shared output.** Two threads printing at once produced interleaved text like `workerworker1 running2 running`, because each `<<` is a separate operation. Guarding the print with a mutex makes each line come out whole. The *order* of threads can still vary; a mutex controls *one at a time*, not *who goes first*.

**A lost-update race.** Two threads each increment a shared `int` 100,000 times, so the correct total is 200,000. `counter++` is a **read-modify-write** (read, add 1, write back). Without synchronization, both threads can read the same value before either writes, so updates are lost.

| Version | Observed results (10 runs) |
|---|---|
| No mutex | ~100,000–105,000, different every run (about half of all increments lost) |
| `std::lock_guard` around `counter++` | 200,000 every run |

It compiled with no warnings and still produced wrong answers, nondeterministically. That's why races are dangerous.

**`std::lock_guard` and RAII.** The guard locks the mutex when constructed and unlocks it when it goes out of scope, so the lock is always released, even on an early return. It's the same ownership idea as `unique_ptr`.

**Trade-off.** When the critical section is almost the entire loop, threads mostly wait on each other, so the "safe" version can be slower than single-threaded code. For a simple counter, `std::atomic<int>` is the lighter-weight tool. Mutexes are for protecting multiple operations or larger structures.

---

# Part 3: Producer/consumer pipeline (`pipeline.cpp`)

```
temp sensor thread     ──push──┐
pressure sensor thread ──push──┼──> [ shared std::queue ] ──pop──> consumer thread ──> Logger
IMU sensor thread      ──push──┘          (mutex)                  (range checks)
```

- **Producers**: one thread per real `Sensor` created by `Sensor::create`. Each calls its sensor's `read()` (polymorphism) every 10 ms and pushes a `Reading{sensorId, value}` into a shared queue, adding a small simulated drift so out-of-range values appear. Each sensor is used by exactly one thread, so the sensors themselves need no locking. Only the shared queue does.
- **The consumer** pops readings in FIFO order, checks each one against its sensor's safe range, and logs it.
- **The queue** decouples data generation from processing, so each side runs at its own pace.

## Concepts

**Structs vs. classes.** `Reading` is a `struct`: plain public data that groups a sensor id with its value so they can't get separated. Classes are used when data has invariants to protect (encapsulation), like the ring buffer's `head` and `count`.

**Critical sections.** Every access to the shared queue is protected by `queueLock`. The producer releases the lock in its own `{ }` scope **before** sleeping, so it never holds the lock while idle.

**Check-then-act.** The consumer's `empty()` check, `front()`, and `pop()` happen under **one** lock. If they were locked separately, another thread could empty the queue between the check and the pop.

**Lock scope matters.** An early version held the lock for the consumer's entire loop. Once threads were involved, producers could never push, so the consumer would wait forever: a hang closely related to deadlock. Taking the lock once per loop iteration fixed it.

**Thread-safe singleton logger.**
```cpp
static Logger& instance() {
    static Logger logger;   // constructed exactly once
    return logger;
}
```
- The **private constructor** guarantees only one `Logger` can exist.
- A **function-local static** is initialized exactly once, and since C++11 that initialization is guaranteed to be thread-safe even if several threads call `instance()` at the same moment.
- `log()` holds the logger's own mutex, so messages from different threads never interleave.
- Centralizing output means format or destination (timestamps, a file, flash memory, a radio link) can change in one place.

**Range-based fault detection.** `isOutlier()` checks each reading against its sensor's safe operating range and logs a `WARNING` for anything outside it:

| Sensor | Safe range |
|---|---|
| Temperature (1) | 20.0 – 24.0 |
| Pressure (2) | 100.0 – 104.0 |
| IMU (3) | 9.0 – 12.0 |

The simulated readings drift upward, so the last two readings of each sensor are flagged (6 in total).

**Busy-waiting.** The consumer polls the queue in a loop, re-checking even when it's empty, which wastes CPU. A check counter makes that polling visible. The standard fix is a `std::condition_variable`: the consumer sleeps in `wait()` (which releases the mutex while sleeping and uses a predicate to handle spurious wakeups), and producers call `notify_one()` after pushing.

**A simplification.** The consumer stops after a fixed number of readings (15). Real systems run indefinitely and use a shutdown flag or a sentinel message instead.

---

## Big-O summary

`n` = number of stored items.

| Structure / operation | Time | Notes |
|---|---|---|
| Ring buffer `push` | **O(1)** | One index calculation and one write. No shifting, and it overwrites at `head` when full. |
| Ring buffer `pop` (remove oldest) | **O(1)** | Not implemented. It would advance `head` and decrement `count`. Compare `vector::erase(begin())`, which is O(n). |
| Ring buffer access i-th oldest | **O(1)** | `data[(head + i) % capacity]` |
| Ring buffer traversal | **O(n)** | Visits each stored reading once |
| Ring buffer memory | **O(capacity)**, fixed | Storage lives inside the object, with no allocation after creation |
| `std::queue` push / pop / front | **O(1)** | Amortized. Memory grows as needed, unlike the ring buffer. |

**Ring buffer vs. `std::queue`.** Both are FIFO. The queue grows without bound and allocates memory. The ring buffer has fixed memory and overwrites old data. Embedded systems often use a ring buffer *as* the inter-thread queue for exactly that reason.

---

## Roadmap

**Next: Linux and Valgrind**
- Build and run all three programs on **Linux (Ubuntu)** inside a Docker container, using the GNU toolchain (`g++ -pthread`).
- Use **Valgrind Memcheck** on the non-virtual-destructor bug. Give a derived sensor a heap-owning member so the skipped destructor produces a reported leak, then confirm a clean report once `virtual` is restored.
- Use **Valgrind Helgrind** on the unsynchronized counter to have the tool detect the data race automatically, then confirm the mutex version is clean.

**Later**
- Replace the consumer's busy-wait with a `std::condition_variable`.
- Replace the fixed reading count with a shutdown flag.
- Demonstrate a two-mutex deadlock and fix it with consistent lock ordering or `std::scoped_lock`.
- Use each sensor's ring buffer to compute a rolling average and detect sudden spikes, rather than only fixed thresholds.
- Use the ring buffer as the shared inter-thread queue, so the pipeline needs no dynamic allocation.
