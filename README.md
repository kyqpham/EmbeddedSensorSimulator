# Embedded Sensor Simulator

A small C++17 program that simulates the sensor layer of an embedded system. Three fake sensors (temperature, pressure, IMU) share one abstract interface, are created through a factory, are stored and polled polymorphically, and each one logs its recent readings into a hand-written fixed-size **ring buffer**.

## Build and run

```bash
clang++ -std=c++17 -Wall -Wextra -g main.cpp -o sim && ./sim
```

Example output:

```
count: 0
thermometer: 21.5 22 22.5 23
pressure: 101.3 101.3 101.3 101.3
imu: 9.8 9.8 9.8 9.8
count: 3
```

The thermometer took 6 readings (20.5 → 23.0), but its buffer holds only 4, so the two oldest were overwritten.

## Structure

| Class / function | Role |
|---|---|
| `RingBuffer` | Fixed-capacity circular buffer of `double` readings (capacity 4) |
| `Sensor` | Abstract base class: the contract every sensor must fulfill |
| `Thermometer`, `PressureSensor`, `IMUSensor` | Concrete sensors that return fake data |
| `Sensor::create(type)` | Static factory: builds a sensor from a text name |
| `report(Sensor&)` | Free function that works on *any* sensor |

---

## Concepts, in the context of this project

### Abstract classes
`Sensor` declares `read()` and `name()` as **pure virtual** (`= 0`), so it has no implementation of its own and cannot be instantiated. `Sensor s;` is a compile error. It exists only as a contract: *to be a sensor, you must be able to read and name yourself.*

A "generic sensor" has no meaningful reading. A default like `return 0.0;` would hide bugs, because a new sensor type that forgot to implement `read()` would silently report 0. Making it pure virtual turns that mistake into a compile error.

### Inheritance
`class Thermometer : public Sensor` means *a Thermometer **is a** Sensor*. Each subclass inherits the shared parts (`category()`, `sample()`, `printLog()`, its log buffer, the instance counter) and supplies its own `read()` and `name()`.

### Virtual functions and polymorphism
Because `read()` is `virtual`, a call through a `Sensor&` or `Sensor*` is resolved at **runtime** based on the object's real type:

```cpp
void report(Sensor& s) { std::cout << s.name() << ": " << s.read(); }
```

`report` is written once and prints the correct value for every sensor type. This also works *inside* the base class: `Sensor::sample()` is a normal, non-virtual function, but it calls the virtual `read()`, so each sensor logs its own data.

**Experiment:** removing `virtual` from `read()` (and its `override`) made `report(thermometer)` print the base version's `0` instead of `21.5`. Without `virtual`, the call is chosen at compile time from the *declared* type (`Sensor&`), not the real one.

### `override`
Every subclass function that replaces a virtual one is marked `override`. The compiler then verifies that a matching virtual function actually exists. During development this caught a `const` mismatch between `Sensor::read()` and `Thermometer::read()`. Without `override`, the mismatch would have compiled silently as a *new* function rather than an override.

### Virtual destructors
`Sensor` has a `virtual ~Sensor()`. Sensors are owned through base-class pointers, so deleting one must run the **derived** destructor first, then the base one:

```
Thermometer destroyed
Sensor destroyed
```

**Experiment:** with a non-virtual destructor, `delete` through a `Sensor*` skipped `~Thermometer()` entirely, so only `Sensor destroyed` printed. clang also warned at compile time. That's undefined behavior, and a real resource leak if the derived class owned anything. **Rule:** any class with virtual functions gets a virtual destructor.

### References and pointers
- `Sensor&` (a reference) lets `report` work on the caller's real object instead of a copy. Passing by value would **slice** a `Thermometer` down to a plain `Sensor`.
- `->` calls a function through a pointer (`s->read()`), and `.` calls it on an object (`log.push(...)`).

### Smart pointers: `std::unique_ptr`
Sensors are stored as `std::vector<std::unique_ptr<Sensor>>`:
- A vector of `Sensor` objects is impossible (abstract) and would slice anyway. Pointers preserve each object's real type.
- `unique_ptr` **owns** its object and deletes it automatically when it goes out of scope, so there's no manual `delete` and no leaks.
- It's **move-only** (one owner, no copies), which is why the loop is `for (const auto& s : sensors)`. Iterating by value would try to copy and fails to compile.

### Statics
- **`static int count`**: one counter shared by the whole `Sensor` class, not per object. `Sensor()` increments it and `~Sensor()` decrements it. Every subclass runs the base constructor and destructor automatically, so it tracks live sensors of *all* types. It's accessed as `Sensor::count`.
- **`static` factory `Sensor::create("temp")`**: a function that belongs to the class and needs no existing object. It maps text to a concrete type and returns `nullptr` for unknown names. It's declared inside `Sensor` but defined *after* the subclasses, since it needs to know about them.
- **`static constexpr int capacity`** in `RingBuffer`: a compile-time constant shared by every buffer.

### Composition ("has-a")
Each `Sensor` **has a** private `RingBuffer log`. Inheritance models *is-a* (a thermometer is a sensor). Composition models *has-a* (a sensor has a log). Each sensor gets its own buffer so readings stay labeled by source, and a fast sensor can't overwrite a slow sensor's history.

### `const` and encapsulation
Read-only functions (`name()`, `size()`, `print()`) are `const`. `read()` is deliberately **not** `const`, because the thermometer updates its internal `temp` on every read. `RingBuffer`'s `data`, `head`, and `count` are `private`, so only its own methods can modify the bookkeeping.

---

## The ring buffer

A fixed array of `capacity` slots used as a circle. When full, a new reading **overwrites the oldest** one. That's ideal for embedded systems: memory use is fixed forever, and the most recent history is always available.

State:
- `head`: index of the **oldest** reading
- `count`: number of stored readings

Key formulas, where `%` wraps the index back to 0:
- next write slot (not full): `(head + count) % capacity`
- i-th oldest reading: `(head + i) % capacity`

Trace with capacity 4:

```
push 10,20,30,40  -> [10 20 30 40]  head=0 count=4   (full)
push 50           -> [50 20 30 40]  head=1 count=4   (overwrote 10)
push 60           -> [50 60 30 40]  head=2 count=4   (overwrote 20)
oldest -> newest:     30 40 50 60
```

## Big-O analysis

`n` = number of stored readings (always ≤ `capacity`).

| Operation | Time | Why |
|---|---|---|
| `push` | **O(1)** | One index calculation and one write. No loop, and no elements are shifted, whether the buffer has 4 slots or 4 million. When full, it overwrites at `head` and advances `head`. |
| `pop` (remove oldest) | **O(1)** | Not implemented here, but it would just advance `head = (head + 1) % capacity` and decrement `count`. Compare `vector::erase(begin())`, which is O(n) because every element shifts left. |
| Access i-th oldest | **O(1)** | Direct index: `data[(head + i) % capacity]`. |
| Traversal (`print`) | **O(n)** | Visits each stored reading exactly once. |
| `size` / `empty` / `full` | **O(1)** | Read a stored counter. |
| Memory | **O(capacity)**, fixed | `std::array` storage lives inside the object. The buffer never grows or allocates after creation, which is the property embedded code wants. |

---

## Roadmap
- Build and run on Linux (Ubuntu via Docker) with `g++`
- Run the non-virtual-destructor experiment under **Valgrind** to see the leak report
- Multithreading: producer/consumer threads sharing a queue, protected with `std::mutex` / `std::lock_guard`
- Thread-safe logger singleton and a deadlock demo fixed with `std::scoped_lock`
