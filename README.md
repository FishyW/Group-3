# MTRX2700 Project 2 - Group 3

## Group Members
- Jack Ryder
- Winston Wijaya
- Denny An
- David Li

---

## Roles and Responsibilities
The project was completed using a modular team structure so that each member could focus on a major subsystem while still contributing to integration and testing.

- **Jack Ryder**  
  Worked on Section 7.4.2 (with the exception of Part C), assisted with Section 7.5.2 integration through board initialisation, and completed the meeting minutes and README.

- **Winston Wijaya**  
  Worked on Section 7.3.2 (UART), Section 7.5.2 integration and debugging, Section 7.4.2 Part C (I2C Advanced - Attitude Estimator), and Section 7.1.2 Part E (Digital I/O Advanced - LED timing).

- **Denny An**  
  Worked on Section 7.2.2 (PWM).

- **David Li**  
  Worked on Section 7.1.2 (Digital I/O).

All members contributed to final integration, debugging, and testing.

---

## Project Overview
This project is a modular embedded systems project for the **STM32F3Discovery** platform. The codebase is organised into separate modules for digital I/O, timers and delays, PWM and servo control, UART communication, I2C communication, SPI communication, sensor drivers, and a final two-board integration task.

The main program is designed so that individual exercise test functions can be enabled or disabled by commenting or uncommenting calls in `main.c`. This made it possible to test each subsystem independently before combining everything into the final integrated system.

At the highest level, the final integrated design uses **Board A** to read magnetometer data and button state, then transmit a `BoardMessage` containing a `MagSample` and display mode over UART. **Board B** receives that message and either maps heading to the LED array or maps heading to servo position.

---

## Exercise 1 - Digital I/O

### Summary
This exercise builds the low-level and reusable digital I/O modules. The `gpio` module provides generic input/output setup, read, write, and toggle functionality. The `led` module wraps the eight onboard LEDs as a software-controlled array, while the `button` module configures the user button on **PA0** with an interrupt callback using **EXTI0**.

An advanced part of this exercise is the LED rate-limiting behaviour. Each LED stores its last update time, and writes can be ignored if they occur before a configurable minimum interval has elapsed.

### Usage
Call `gpio_init()` before using a pin, `led_init()` before controlling the board LEDs, and `button_init(callback)` before using the user button interrupt.

For LED-only testing, uncomment:
- `testLedSpeedLimiter()`

in `main.c`.

### Valid input
- Valid GPIO pins: `0` to `15`
- Valid GPIO modes: input or output
- Valid LED identifiers: `LED0` to `LED7`
- `led_set_all()` accepts an 8-bit bitmask representing all LED states
- The button module accepts a callback of type `void (*)(void)`

### Functions and modularity
Relevant functions include:
- `gpio_init`
- `gpio_write`
- `gpio_read`
- `gpio_toggle`
- `led_init`
- `led_set`
- `led_get`
- `led_get_all`
- `led_set_all`
- `button_init`
- `button_is_pressed`

This separation allows other modules to interact with LEDs and button logic without repeatedly configuring raw STM32 registers.

### Testing
The main provided test function is:
- `testLedSpeedLimiter()`

This repeatedly updates the LED mask to demonstrate that the rate limiter prevents excessively fast LED changes.

Manual testing can also be done by:
- verifying LED bitmask behaviour
- confirming that the EXTI button callback toggles software state correctly when pressed

### Notes
The LED module stores a software copy of LED state, so other modules can query LED values without directly reading hardware registers. The button interrupt is configured on the rising edge of PA0 using EXTI0 and NVIC.

---

## Exercise 2 - Timer Interface

### Summary
This exercise implements reusable timer functionality using STM32 timers, then builds high-level delay utilities and PWM-based servo control on top of it. The timer module supports periodic callbacks, one-shot timing, elapsed time measurement, and delay functions. The PWM module uses timer callbacks to alternate between rising and falling edges, and the servo module converts an angle command into a pulse width for **PB6**.

The servo logic maps angles in the range **0 to 90 degrees** into pulse widths from **1 ms to 2 ms**, with a PWM period of **20 ms**.

### Usage
Use `timer_init()` to configure a timer with a callback and period, `delay()` or `delayElapsed()` for blocking delays, and `servoInit()` followed by `servoWrite(angle)` to drive the servo.

For testing, uncomment:
- `testDelay()`
- `testServo()`

in `main.c`.

### Valid input
- Timer periods can be configured in milliseconds or microseconds depending on the function used
- `delay()` and `delayElapsed()` take millisecond values
- `servoWrite()` expects a floating-point angle
- The servo mapping is designed around a `0` to `90` degree input range

### Functions and modularity
Key timer functions include:
- `timer_init`
- `timer_set_period_us`
- `timer_set_period`
- `timer_oneshot_call`
- `timer_irq`
- `getNow`
- `getElapsed`
- `delay`
- `delayElapsed`

The PWM layer provides:
- `pwm_init`
- duty / period setters

The servo layer provides:
- `servoInit`
- `servoWrite`

This layered structure keeps timing, waveform generation, and actuator control separate.

### Testing
The test functions used are:
- `testDelay()`  
  Increments the LED display once per second to demonstrate timer-based delay behaviour.

- `testServo()`  
  Sweeps the servo continuously by updating angle over time.

The timer module also includes demonstration behaviour for:
- periodic callbacks
- changing timer periods
- one-shot timing

### Notes
The elapsed-time helper uses `TIM2_TIMER` as a long-running timing base, while PWM-related timing uses `TIM3_TIMER`. This separates general software timing from the PWM generation path.

---

## Exercise 3 - Serial Interface 

### Summary
This exercise implements asynchronous UART transmit and receive logic with interrupt handling, packet framing, checksum validation, and a double-buffer receive design. Messages are framed as:

`STX <size> <message id> <payload> ETX <checksum>`

The receive path validates message size and checksum before calling a completion callback.

The UART module supports both simple string transmission and structured byte-payload transmission. It also handles receive overflow, checks termination, and protects data consistency by copying validated payloads out of the shared receive buffer before invoking the callback.

### Usage
Call `serialInitialise()` with a port, baud rate enum, and optional receive callback. Use `sendString()` for text and `sendMsg()` for structured or binary payloads. Use `receiveMsg()` when the main application wants the latest completed message.

For testing, uncomment:
- `testSerial()`
- `testSerialString()`

in `main.c`.

### Valid input
- Valid serial ports:
  - `USART1_PORT`
  - `UART4_PORT`
- Valid baud selections:
  - `BAUD_9600`
  - `BAUD_19200`
  - `BAUD_38400`
  - `BAUD_57600`
  - `BAUD_115200`
- `sendMsg()` accepts:
  - `uint8_t * payload`
  - payload size
  - message ID
- `sendString()` accepts a null-terminated C string

### Functions and modularity
Important functions include:
- `serialInitialise`
- `sendString`
- `sendMsg`
- `receiveMsg`

These are supported internally by interrupt-driven transmit and receive helpers. The module hides `SerialPort` internals in the `.c` file so other modules only interact with the public API.

### Testing
The test functions used are:
- `testSerial()`  
  Repeatedly sends a framed `"hello\r\n"` message and waits for a received message while incrementing the LED display through the callback.

- `testSerialString()`  
  Repeatedly sends a plain `"hello\r\n"` string once per second.

### Notes
A strong feature of this module is the double-buffered receive design, which helps avoid race conditions between interrupt-driven incoming data and foreground code reading the latest complete message. The validated payload is copied into a temporary buffer before the callback is invoked so later buffer changes do not corrupt processed data.

---

## Exercise 4 - I2C Sensor Interfacing + SPI Attitude Estimator

### Summary
This exercise covers the sensor subsystem. The gyroscope is read over SPI, while the accelerometer and magnetometer are read over I2C.

The magnetometer driver configures the **LSM303AGR** for continuous conversion at **100 Hz** with:
- low-pass filtering
- offset cancellation
- block-data-update

It then computes heading in degrees from the X and Y axes.

The accelerometer driver reads 12-bit left-aligned X/Y/Z data and converts it to floating-point values. The gyroscope driver reads angular velocity and converts raw readings to degrees per second. The attitude estimator combines accelerometer, gyroscope, and magnetometer information to estimate roll, pitch, and yaw, using a stability measure over a circular buffer to decide whether to trust gyro integration or the accel/magnetometer estimate.

### Usage
Use:
- `initializeGyro()`
- `initializeAccel()`
- `magInit()`

before reading sensors.

Use:
- `readGyro()`
- `readAccel()`
- `magReadSample()`

to obtain measurements.

For testing, uncomment:
- `testGyro()`
- `testAccel()`
- `testAttitude()`
- `testMagnetometer()`

in `main.c`, depending on which subsystem is being checked.

### Valid input
The sensor read functions take pointers to destination structs such as:
- `GyroRawData`
- `AccelRawData`
- `MagSample`

The low-level communication layers accept:
- device addresses
- register addresses
- destination buffers
- transfer lengths

The I2C API supports both single-register and multi-register reads, with optional auto-increment.

### Functions and modularity
The sensor layer is split by device and bus.

SPI support includes:
- `initializeSPI`
- `readSPIRegister`
- `writeSPIRegister`

I2C support includes:
- `i2c1Init`
- `i2cWriteReg`
- `i2cReadReg`
- `i2cReadRegs`

Sensor-specific functions include:
- `initializeGyro`
- `readGyro`
- `initializeAccel`
- `readAccel`
- `magInit`
- `magReadSample`

The higher-level attitude estimator sits above all three sensors.

### Testing
The test functions used are:
- `testGyro()`  
  Integrates angular velocity into angles and sends values over UART roughly every 200 ms.

- `testAccel()`  
  Reads and prints acceleration values at a rate-limited interval.

- `testAttitude()`  
  Prints roll, pitch, and yaw while updating the estimate every 10 ms.

- `testMagnetometer()`  
  Used to verify magnetometer initialisation, raw sample acquisition, and heading output during standalone development and integration.

### Notes
The heading calculation uses `atan2f` on the processed X and Y magnetometer values, so the exact heading convention depends on board and sensor mounting orientation. The attitude estimator also includes tilt compensation for yaw and a rolling stability metric before choosing between gyro-based integration and accel/magnetometer-based orientation.

---

## Exercise 5 - Integration Task

### Summary
The integration task links both boards into a complete system.

- **Board A** reads magnetometer data, tracks a display mode selected by the user button, and sends both items to Board B in a `BoardMessage`.
- **Board B** receives the message and either displays compass direction on the LED array or drives the servo based on heading.

On Board A, the button interrupt toggles a software state and updates local indicator LEDs so the current mode is visible. On Board B, `writeHeadingToLEDs()` maps heading ranges to one of eight compass directions, while `writeHeadingToServo()` converts the received heading into a mirrored `0` to `90` degree servo angle.

### Usage
To run the integrated system:
- use `runBoardA()` on the transmitting board
- use `runBoardB()` on the receiving board

In `main.c`, only one of these should be active at a time depending on which board is being programmed.

### Valid input
The integration interface is the `BoardMessage` structure, which contains:
- a `MagSample`
- an 8-bit `displayState`

UART4 is used for communication at:
- `BAUD_57600`

Board A transmits roughly every 10 ms after reading a fresh magnetometer sample.

### Functions and modularity
Board A is mainly composed of:
- `initializeBoardA`
- `buttonPressed`
- `updateModeLeds`
- `runBoardA`

Board B is mainly composed of:
- `initializeBoardB`
- `receiveCallback`
- `writeHeadingToLEDs`
- `writeHeadingToServo`
- `runBoardB`

This keeps sensor acquisition, communication, and output handling separated into clean stages.

### Testing
Integration testing can be done by flashing Board A and Board B separately, then observing whether button presses on Board A change the display mode on Board B.

Correct operation is shown when:
- heading updates light the appropriate directional LED in LED mode
- heading updates reposition the servo in servo mode

The code also contains commented mock-message examples in Board B for debugging receive behaviour without live sensor input.

### Notes
One implementation detail worth noting is that comments and logic may differ on how `displayState` is interpreted. The actual code behaviour should be treated as the source of truth.

The project also includes `syscalls.c` and `sysmem.c`, which are STM32CubeIDE support files for standard library and heap behaviour rather than core exercise modules.

---

## Usage
1. Open the project in **STM32CubeIDE**.
2. Build the project for the STM32F3Discovery board.
3. Flash the correct firmware onto Board A or Board B.
4. Uncomment the required test function or runtime function in `main.c`.
5. Run the program and observe the relevant subsystem behaviour through LEDs, UART output, sensor readings, or servo movement.

---

## Valid input
The project expects:
- valid STM32F3Discovery hardware setup
- correct peripheral clock and pin configuration
- correct I2C and SPI device addressing
- valid UART configuration and wiring
- valid servo angle inputs in the intended `0` to `90` degree range
- valid structured message formatting for UART packet transmission

---

## Functions and modularity
The project was designed using separate modules so that each subsystem could be developed and tested independently.

Typical modules include:
- `gpio.c / gpio.h`
- `led.c / led.h`
- `button.c / button.h`
- `timer.c / timer.h`
- `pwm.c / pwm.h`
- `servo.c / servo.h`
- `serial.c / serial.h`
- `i2c.c / i2c.h`
- `spi.c / spi.h`
- sensor driver files
- board integration files

Important test and runtime functions include:
- `testLedSpeedLimiter()`
- `testDelay()`
- `testServo()`
- `testSerial()`
- `testSerialString()`
- `testGyro()`
- `testAccel()`
- `testAttitude()`
- `testMagnetometer()`
- `runBoardA()`
- `runBoardB()`

This modular structure improves:
- readability
- debugging
- maintainability
- reuse of code between exercises and final integration

---

## Testing
Testing was carried out progressively at both module level and full-system level.

### Module Testing
- LED and button abstraction testing through `testLedSpeedLimiter()`
- timer and delay testing through `testDelay()`
- PWM and servo testing through `testServo()`
- UART testing through `testSerial()` and `testSerialString()`
- gyroscope testing through `testGyro()`
- accelerometer testing through `testAccel()`
- attitude estimation testing through `testAttitude()`
- magnetometer testing through `testMagnetometer()`

### Integration Testing
Full-system testing was performed by programming Board A and Board B separately and checking whether:
- button presses changed display mode correctly
- UART messages were transmitted and received correctly
- heading values were displayed correctly on the LED array
- heading values were converted correctly to servo movement

### Notes
The project was developed with a strong emphasis on modularity, debugging, and incremental integration. Using dedicated test functions in `main.c` made it easier to isolate faults in each subsystem before attempting the final two-board integration.

---

## Notes
- The project follows a modular embedded systems design approach.
- Individual test functions can be enabled or disabled in `main.c`.
- UART output was used heavily for debugging and observing sensor/system behaviour.
- The final system demonstrates communication between two STM32F3Discovery boards using structured messages and sensor-driven output modes.
