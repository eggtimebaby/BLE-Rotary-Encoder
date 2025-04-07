# ESP32 BLE Encoder Control

An enhanced Bluetooth LE keyboard controller using a rotary encoder with the ESP32-C3 microcontroller. This project turns a rotary encoder into a wireless keyboard that sends arrow key presses and enter key commands.

## Current Status

This project is under active development. The current version works with USB power and has improved debouncing and BLE connection handling.

## Features

- **Robust BLE Connection Handling**: Improved connection management with automatic reconnection
- **Enhanced Debouncing**: Proper debouncing for both encoder rotation and button presses
- **Visual Feedback**: RGB LED status indicator for connection state and activity

## Hardware Requirements

- ESP32-C3 development board
- Rotary encoder with push button
- WS2812 RGB LED (NeoPixel)

## Wiring

| Component | ESP32-C3 Pin |
|-----------|--------------|
| Encoder CLK | GPIO 2 |
| Encoder DT | GPIO 1 |
| Encoder SW (Button) | GPIO 0 |
| RGB LED Data | GPIO 10 |

## Dependencies

- [ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) library
- [FastLED](https://github.com/FastLED/FastLED) library

## LED Status Indicators

| Color | Meaning |
|-------|---------|
| Red | Disconnected |
| Blinking Orange | Attempting to connect |
| Green | Connected |
| Blue (momentary) | Activity (encoder rotation or button press) |

## Improvements Over Original Version

1. **Better Debouncing**
   - Time-based filtering for encoder signals
   - State machine for button debouncing
   - Prevents duplicate key presses from a single physical action

2. **Enhanced BLE Connection**
   - Custom BLE connection callbacks
   - Automatic reconnection attempts
   - Clear visual feedback of connection state

3. **Code Structure**
   - Modular design with separate functions for different responsibilities
   - Improved state management
   - Better serial logging for debugging

## Usage

1. Install the required libraries in Arduino IDE
2. Connect the hardware according to the wiring table
3. Upload the sketch to your ESP32-C3
4. Pair with your computer or mobile device
5. Use the rotary encoder to send left/right arrow keys
6. Press the encoder button to send Enter key

## Troubleshooting

- **Multiple inputs from one action**: If you still experience multiple inputs from a single encoder click, try increasing the `encoderDebounceTime` value (currently set to 5ms)
- **Button issues**: If button presses are not reliable, try adjusting the `buttonDebounceTime` value (currently set to 50ms)
- **Connection problems**: Check the LED status indicator. If it's blinking orange for a long time, try restarting both the ESP32 and your Bluetooth device

## Planned Improvements

### Battery Support
- Add LiPo/Li-ion battery support with voltage monitoring
- Implement battery level reporting via BLE
- Add low battery warning indicators
- Add charging status monitoring and indicators

### Power Management
- Implement deep sleep mode for battery conservation
- Add wake-on-motion or wake-on-button functionality
- Add configurable auto-sleep timeout
- Optimize power consumption during active use

### Feature Enhancements
- Add configurable key mappings
- Support for long-press and double-click actions
- Add configuration mode for settings adjustment
- Add macro support for common key combinations
- Consider adding acceleration for fast encoder rotation

### Hardware Improvements
- Design custom PCB for more compact form factor
- Add battery charging circuit
- Consider adding additional buttons for more functions
- Explore using magnetic encoder for better feel
- Add optional OLED display for status information

### Software Improvements
- Add OTA (Over-The-Air) firmware updates
- Implement configuration storage in flash memory
- Add diagnostic mode for troubleshooting
- Improve BLE reconnection reliability
- Add support for multiple device pairing

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.
