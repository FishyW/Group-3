# MTRX Group 3 - "Completionists"
## Date - 14/04/2026

### Present:
- Jack Ryder
- Winston Wijaya
- Denny An
- David Li

### Meeting Purpose:
To review progress made before 14/04/2026, update task allocations, and confirm the status of each question before continuing development.

### Past Items:
- Most Header File have been completed, Winston has pushed his onto to the GIT page, others have not.

### Current Items:
- GPIO module has been completed as well as LED and Button interfacing and I/O (Jack)
- Timer Module Created, Timer reset function completed, PWM is Work in Progress (Denny)
- All UART components have been completed and modularised  (Winston)
  
### New Items:
- Jack Ryder and David Li swapped question allocations, with David to complete parts D and E.
- Shared STM32 header/configuration setup was added to simplify project setup
- UART progress and testing functions were discussed
- Timer progress was reported

### Progress Checklist:

#### Question 1 - Digital I/O (David Li)
This question focuses on creating reusable modules for GPIO, LEDs, and button input on the STM32F3 Discovery Board. - (Jack Ryder)
- [x] Part A: Generic GPIO module for input/output pin setup and read/write access - (Jack Ryder)
- [x] Part B: Separate button and LED interface modules built on the GPIO module - (Jack Ryder)
- [x] Part C: Button callback function using a function pointer
- [ ] Part D: LED state encapsulated through get/set functions only
- [ ] Part E: Advanced LED speed restriction using a timer without polling delay

#### Question 2 - Timer Interface (Denny An)
This question focuses on building a hardware timer module, including periodic callbacks, timer reset behaviour, PWM generation, and one-shot timing events.
- [x] Part A: Timer module that triggers a callback at a regular interval.
- [x] Part B: Timer reset function with get/set access for the timer period
- [ ] Part C: PWM generation at 50 Hz for hobby servo control
- [ ] Part D: One-shot timer event with delay and callback function

#### Question 3 - Serial Interface (Winston Wijaya)
This question focuses on UART communication for sending and receiving structured data, as well as debug strings and interrupt-based serial handling.
- [x] Part A: UART module for sending and receiving arrays of bytes
- [x] Part B: `sendString` function for serial debugging output (Winston Wijaya)
- [x] Part C: `sendMsg` function for structured message transmission
- [x] Part D: `receiveMsg` function with checksum validation and callback
- [x] Part E: Interrupt-based serial receiving
- [ ] Part F: Interrupt-based transmitting with double buffering

#### Question 4 - I2C Sensor Interfacing (Jack Ryder)
This question focuses on building an I2C interface for the Discovery Board compass module and storing the returned magnetometer data in a shared structure.
- [ ] Part A: I2C magnetometer module
- [ ] Part B: Data structure containing raw magnetometer values, decoded heading, and timestamp
- [ ] Part C: Advanced attitude estimation using accelerometer and gyro data

#### Question 5 - Integration Task (All Members)
This question focuses on combining all modules into a full working system across two STM32 boards using magnetometer data, UART communication, button input, servo output, and LEDs.
- [ ] Part A: Read magnetometer data on STM32 Board 1 and send it over UART to STM32 Board 2
- [ ] Part B: Use an interrupt-driven button input on Board 1 to change display mode
- [ ] Part C: Use Board 2 to control servo position and LED heading display based on received data

### Next Meeting Date:
[19/04/2026]
