# MTRX2700 Group 11 | Meeting Minutes

**Date:** [31/03/2026]  
**Time:** [3:00 pm]  
**Location:** [Mechatronics Dry Lab | Link Building Room 330]  
**Recorded by:** Jack Ryder  

## Attendees
- Jack Ryder
- Winston Wijaya
- Denny An
- David Li

**Apologies/Absences:** None

---

## Agenda
1. Go through the assignment requirements  
2. Plan the modular structure of the code  
3. Assign roles for each exercise  
4. Set a timeline for completion  

---

## Discussion Summary

### 1. Assignment Overview
The group reviewed the full assignment and agreed to complete it in a modular way, building and testing each software component separately before attempting the final integration task.

### 2. Modularity Plan
The group agreed that each major function of the assignment should be separated into its own module. This will make the code easier to test, debug, and integrate later.

Proposed modules:
- **GPIO module** for button and LED interfacing
- **Timer module** for delays, callback timing, and PWM generation
- **UART module** for serial communication
- **I2C module** for magnetometer/compass communication
- **Sensor module** for handling magnetometer data structures and processing
- **Main/integration file** to combine all modules in Exercise 5

### 3. Exercise Breakdown

#### Exercise 1: Digital I/O
The group discussed creating a module that can:
- configure GPIO pins as input or output
- perform digital read and write operations
- support button and LED interfacing
- potentially use callback functions for button events
- prevent LEDs from changing too quickly by adding a cooldown/throttle if required

The group agreed this exercise should focus on building a reusable GPIO interface first, then wrapping it in simpler LED/button-specific functions.

#### Exercise 2: Timer Interface
The group discussed creating a timer module that can:
- trigger callback functions after a set period
- support adjustable timing
- support one-shot events
- generate PWM signals for servo motor control

The timer module will be important for both Exercise 2 and the final integration task.

#### Exercise 3: Serial Interface
The group discussed creating a UART module that can:
- send and receive bytes
- send strings over USART1
- transmit a structured packet with start byte, message type, body, stop byte, and checksum
- receive structured packets and respond using a callback
- later replace polling with interrupts for receive and transmit

#### Exercise 4: I2C Sensor Interfacing
The group discussed implementing I2C communication for the magnetometer/compass and storing returned data in a structure.

The group also noted that accelerometer data may later be used for attitude estimation.

#### Exercise 5: Integration Task
The group identified that the final task will combine all previous modules:
- one board reads magnetometer data
- data is packaged into a structure and sent over UART
- a button interrupt changes the display mode
- the second board displays data using either LEDs or servo position

---

## Decisions Made
- The assignment will be completed **module by module** before full integration.
- Each module should be written so it can be tested independently.
- Callback functions will be used where useful, especially for buttons, timers, and serial receive events.
- Exercise 5 will only begin after the GPIO, timer, UART, and I2C modules are functioning individually.
- Progress will be tracked by assigning responsibility for each module to a group member.

---

## Task Allocation / Who Is Doing What

| Team Member | Assigned Work |
|---|---|
| **Jack Ryder** | Completion of Q1
| **Denny An** | Completion of Q2
| **Winston Wijaya** | Completion of Q3
| **David Li** | Completion of Q4
| **All members** | Completion of Q5

---

## Timeline
- **Stage 1:** Complete GPIO, timer, and UART core modules
- **Stage 2:** Complete I2C magnetometer interfacing
- **Stage 3:** Test each module individually
- **Stage 4:** Integrate all modules for Exercise 5
- **Stage 5:** Debug, refine, and prepare final submission

---

## Action Items

| Action Item | Person Responsible | Due Date | Status |
|---|---|---|---|
| Review full assignment requirements and confirm module list | All members | [31/03/2026] | Complete |
| Create base GPIO module for digital input/output | Jack | [05/04/2026] | Not started |
| Create LED/button wrapper functions using GPIO module | Jack | [05/04/2026] | Not started |
| Develop timer module with configurable callback timing | Denny | [05/04/2026] | Not started |
| Investigate PWM requirements for servo control | Denny | [05/04/2026] | Not started |
| Develop UART send/receive functions | Winston | [05/04/2026] | Not started |
| Design serial packet structure with checksum | Winston | [05/04/2026] | Not started |
| Review magnetometer I2C requirements | David | [05/04/2026] | Not started |
| Begin integration plan for Exercise 5 | All members | [12/04/2026] | Not started |

---

## Progress Checklist

### Exercise 1: Digital I/O
- [ ] GPIO input/output setup
- [ ] Digital read/write functions
- [ ] LED interface
- [ ] Button interface
- [ ] Callback/event behaviour

### Exercise 2: Timer Interface
- [ ] Timer callback functionality
- [ ] Adjustable timing/reset logic
- [ ] PWM generation
- [ ] One-shot event support

### Exercise 3: Serial Interface
- [ ] Byte send/receive
- [ ] String transmission
- [ ] Structured packet transmission
- [ ] Structured packet receive
- [ ] Interrupt-based receive
- [ ] Interrupt-based transmit

### Exercise 4: I2C Sensor Interfacing
- [ ] I2C communication
- [ ] Magnetometer data structure
- [ ] Accelerometer/attitude estimation support

### Exercise 5: Integration
- [ ] Send magnetometer data over UART
- [ ] Button interrupt changes display mode
- [ ] Servo/LED response to heading data

---

## Next Meeting
**Date:** [08/04/2026]  
**Focus:** Begin programming for all core components of assignment
