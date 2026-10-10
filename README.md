# Multithreaded Sensor Monitoring with Semaphores (C, pthreads)

A multithreaded C program that simulates an environmental monitoring system. Two independent subsystems, **temperature** and **humidity**, each have two sensors producing readings and a controller that averages the latest readings and switches an actuator (heater / dehumidifier) on or off.

The focus is on **thread synchronization with semaphores**: mutual exclusion on shared circular buffers and a producer–consumer notification between sensors and controllers.

> Project developed for the course **Computer Systems Architecture**, BSc in **Electrical and Computer Engineering**, **Instituto Politécnico do Cávado e Ave (IPCA)**, **2st year**.

---

## Repository structure

```
.
├── cdoe.c
└── README.md
```

---

## Architecture

Each subsystem runs in its own thread, which in turn creates three threads: two sensors and one controller.

```
main
├── processo_temperatura
│   ├── temp_sensor1        1 reading every 3 s (25 readings)
│   ├── temp_sensor2        1 reading every 5 s (30 readings)
│   └── temp_controlador    average of the last 5 readings -> heater
└── processo_humidade
    ├── hum_sensor1         1 reading every 8 s (15 readings)
    ├── hum_sensor2         1 reading every 8 s (25 readings)
    └── hum_controlador     average of the last 5 readings -> dehumidifier
```

Sensor readings are simulated with random values between `1.0` and `100.9`.

### Shared data

Each subsystem has a **circular buffer** of `BUFFER_SIZE = 15` floats and a write index. The index only grows, and the slot written is `index % BUFFER_SIZE`, so the oldest value is overwritten once the buffer is full. Both sensors of a subsystem write into the same buffer.

### Synchronization

| Semaphore | Initial value | Purpose |
|---|---|---|
| `sem_tempBuffer`, `sem_humBuffer` | `1` | Binary semaphores used as **mutexes**. They protect the buffer, its index and the counter of finished sensors |
| `sem_temp`, `sem_hum` | `0` | **Counting semaphores** for notification. A sensor posts after each reading, and the controller waits on it, so the controller only wakes up when there is something new to process |

How a controller reads the data:

1. Wait on the notification semaphore (blocked until a sensor posts).
2. Take the buffer mutex and **copy the buffer and index into local variables**, then release the mutex.
3. Compute the average of the last 5 readings on the local copy, so the lock is held for as short a time as possible.
4. Decide the actuator state from the average.

### Actuator control

| Subsystem | Condition | Action |
|---|---|---|
| Temperature | average < 18 | Heater `[ON]`, otherwise `[OFF]` |
| Humidity | average > 60 | Dehumidifier `[ON]`, otherwise `[OFF]` |

The actuator functions only print when the state **changes**, which keeps the output readable. If fewer than 5 readings exist yet, the average uses only the available ones.

### Clean shutdown

When a sensor finishes its readings, it increments a "finished sensors" counter (under the buffer mutex) and posts the notification semaphore one last time. The controller checks that counter every time it wakes up, and when both sensors are done it leaves its loop. The parent threads then `pthread_join` everything and `main` destroys the semaphores.

The full run takes roughly 200 seconds, since the slowest sensor (`hum_sensor2`) takes 25 readings × 8 s.

### Concepts demonstrated

- Threads with `pthread_create` / `pthread_join`
- Binary semaphores as mutexes, and counting semaphores for signalling
- Producer–consumer pattern with multiple producers and one consumer
- Circular buffers
- Minimizing the time spent inside a critical section (copy, then compute)
- Orderly termination of cooperating threads

---

## Example output

Values are random and the interleaving varies between runs:

```
T1: Sensor 1 - Temperatura: 15.3
T3: Media das ultimas 5 temperaturas: 15.3

Aquecedor [ON]
H1: Sensor 1 - Humidade: 71.4
H3: Media das ultimas 5 humidades: 71.4

Desumidificador [ON]
```

Line prefixes: `T1`/`T2` and `H1`/`H2` are the sensors, `T3` and `H3` are the controllers.

---

## Building and running

The program targets **Windows** and uses the Win32 API (`Sleep`) together with a Windows build of POSIX threads and semaphores (`pthread.h`, `semaphore.h`).

1. Create a console project in **Visual Studio** and add `main.c`.
2. Install the NuGet package `aerospike-client-c-dependencies` (version `1.0.2`). The top of `main.c` links the static `pthreadVC2.lib` from that package through `#pragma comment(lib, ...)`. If your pthreads library is somewhere else, change the `PTHREADS_STATIC_LIB_LOCAL_PATH` macro.
3. Build for **x64 / Release** and run.

To port it to Linux or macOS, remove the Windows-specific block at the top of the file and replace `Sleep(ms)` with `usleep(ms * 1000)`.

---

## Limitations

- **Windows only**, as described above.
- **The simulation is open-loop:** the actuators only print their state. The `+0.5` / `+1` applied when they are on affects a local copy, so it never feeds back into the sensor readings.
- Thresholds (18 and 60), the buffer size (15) and the averaging window (5) are hard-coded in the source.
- The average mixes the latest readings from both sensors of a subsystem, regardless of which sensor produced them.
- Comments and messages in the source code are in Portuguese.

---

## Author

**Ivo Azevedo**
