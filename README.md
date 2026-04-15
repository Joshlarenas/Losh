# Losh
Embedded Lab Project (Flight Simulator)

## Description

Losh is a C program for the MSP430 microcontroller that implements a flight simulator input system. It reads sensor data from two sources — an accelerometer and a joystick — and outputs directional information over UART. A buzzer driven by Timer B PWM provides audio feedback when inputs exceed defined thresholds.

**Accelerometer mode (default):**
- Reads X, Y, and Z axes via ADC12 from pins P8.4, P8.5, and P8.6
- Prints raw 12-bit ADC values for each axis over UART at 9600 baud
- Activates the buzzer when X or Y readings fall outside the range 1300–2700

**Joystick mode:**
- Reads X-axis from P9.2 (A10) and Y-axis from P8.7 (A4)
- Calls `print_direction()` to output a cardinal or diagonal direction string (e.g., `Left`, `Top-Right`, `Center`) along with raw ADC values
- Activates the buzzer when X or Y readings fall outside the range 900–3100

Pressing button S1 (P1.1) toggles between accelerometer mode and joystick mode with debounce handling.

The buzzer is connected to P2.7 and driven by Timer B0 CCR6 in PWM mode (`OUTMOD_7`) at 1000 Hz with a 50% duty cycle.

The code also includes `Initialize_I2C()` and I2C read/write functions targeting an OPT3001 light sensor at address `0x44`, though these are not called from `main()`.

## Hardware

- MSP430 microcontroller
- Accelerometer on P8.4–P8.6 (ADC channels A7, A6, A5)
- Joystick on P9.2 (A10) and P8.7 (A4)
- Buzzer on P2.7 (TB0.6 PWM output)
- Button S1 on P1.1 (pull-up)
- UART via eUSCI_A1 on P3.4/P3.5 at 9600 baud

## Dependencies

- `msp430.h` (MSP430 peripheral register definitions)
