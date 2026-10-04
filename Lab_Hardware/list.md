# Desktop HILS, MBD & IoT Hardware Architecture List

| Category | Common Hardware Name | Specific Model | Project Purpose (MBD / HILS / DSP / IoT) |
| :--- | :--- | :--- | :--- |
| **Core Controller** | Microcontroller Development Board | **STM32 NUCLEO-F446RE** | The "Brain" of the HILS system. Runs Simulink-generated code, DSP algorithms, and controls all peripherals. |
| **IoT Node / Telemetry** | ESP32 Development Board | **NodeMCU ESP32 (ESP-WROOM-32)** | Dual-core 32-bit MCU with Wi-Fi/Bluetooth for wireless data telemetry, OTA updates, and IoT dashboard integration. |
| **Communication** | USB to CAN Analyzer | **Makerbase CANable V2.0** | PC host interface (CandleLight/Slcan). Injects commands and sniffs CAN bus traffic for vehicle network simulation. |
| **Communication** | CAN Bus Transceiver Module | **SN65HVD230 (VP230)** | Converts 3.3V logic signals from the STM32 into differential physical CAN bus signals. |
| **Perception (ADAS)** | Time-of-Flight (ToF) Laser Sensor | **TOF53200 (VL53L0X)** | Distance measurement for Automatic Emergency Braking (AEB) and Forward Collision Warning (FCW) logic. |
| **Perception (Dynamics)**| 3-Axis Accelerometer | **GROVE - I2C 3-Axis Accelerometer (MMA7660FC)**| Measures physical acceleration and tilt via I2C for vehicle body dynamics and attitude monitoring. |
| **Perception (Cabin)** | Analog Temperature Sensor | **GROVE - Temperature Sensor** | Plug-and-play analog thermal sensor for cabin climate control (HVAC) and environmental monitoring prototyping. |
| **Actuator (ADAS)** | 180-Degree Micro Servo Motor | **SG90 (180° version)** | Simulates physical ADAS interventions, such as automatic brake pedal actuation or steering angle adjustments. |
| **Audio / DSP** | I2S Digital MEMS Microphone | **INMP441** | Captures high-fidelity digital audio directly to memory (via DMA) for Fast Fourier Transform (FFT) frequency analysis. |
| **Plant (Powertrain)** | DC Gear Motor with Hall Encoder | **GA12-N20 (3V, 150/250 RPM)**| The physical "Plant" for closed-loop PID control, simulating vehicle powertrain dynamics and speed feedback. |
| **Power Driver** | Dual DC Motor Driver Board | **TB6612FNG (Soldered)** | Amplifies the low-voltage PWM signals from the MCU to safely drive the N20 motor via H-bridge logic. |
| **Input Sensor** | Dual-axis Analog Joystick Module | **KY-023** | Acts as a dual-channel electronic throttle pedal for redundant ADC input validation (Anti-surge protection). |
| **Power Supply** | 4-Slot AA Battery Holder | **4xAA Box with Dupont Female** | Provides isolated, safe external power (up to 6V) for the motor and servo, protecting the logic circuit. |
| **Prototyping** | Full-Size Solderless Breadboard | **EIC-801 / MB-102 (830 pts)** | Platform for building the circuit with strict physical isolation between the low-voltage logic and high-current domains. |
| **Wiring** | Jumper Wires & Adapter Cables | **Dupont (M-M, M-F, F-F) & Grove**| Solderless electrical connections linking modules, sensors, and the main controller boards. |