# ESP32 BLE Jog/Scrub Controller

An enhanced Bluetooth LE controller using dual rotary encoders with the ESP32-C3 microcontroller. This project creates a wireless jog/scrub controller for video and audio editing, featuring simultaneous keyboard and mouse functionality for precise timeline control and editing operations.

## Current Status

This project is under active development. The current version works with USB power and has improved debouncing and BLE connection handling.

## Features

### Dual Encoder Support
- **First Encoder**: Timeline scrubbing with Left/Right arrow keys and Enter
- **Second Encoder**: Vertical navigation/zoom with Up/Down arrows and Command

### Enhanced Functionality
- **Robust BLE Connection**: Improved connection management with automatic reconnection
- **Advanced Debouncing**: Proper debouncing for both encoder rotation and button presses
- **Visual Feedback**: RGB LED status indicator for connection state and activity
- **Independent State Tracking**: Separate state management for each encoder

## Hardware Requirements

- ESP32-C3 development board
- 2× Rotary encoders with push buttons
- WS2812 RGB LED (NeoPixel)

## Wiring

### First Encoder (Timeline Scrubbing)
| Component | ESP32-C3 Pin |
|-----------|--------------|
| CLK | GPIO 2 |
| DT | GPIO 1 |
| SW (Button) | GPIO 0 |

### Second Encoder (Vertical Navigation/Zoom)
| Component | ESP32-C3 Pin |
|-----------|--------------|
| CLK | GPIO 6 |
| DT | GPIO 7 |
| SW (Button) | GPIO 8 |

### LED
| Component | ESP32-C3 Pin |
|-----------|--------------|
| Data | GPIO 10 |

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
5. Use the encoders:
   - First encoder: Timeline scrubbing (Left/Right)
   - First button: Enter/Play-Pause
   - Second encoder: Vertical navigation/zoom
   - Second button: Command key for modifiers

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
- Add configurable jog/scrub sensitivity
- Support for variable speed scrubbing
- Add fine-control modifier combinations
- Add macro support for common editing operations
- Implement acceleration for fast timeline navigation

### Hardware Improvements
- Add 0.96" OLED display for:
  * Timeline position/markers
  * Current jog speed/direction
  * Active modifiers/functions
  * Battery and connection status
- Add 3-4 programmable buttons for:
  * Mouse buttons for editing operations
  * Transport controls (play/pause, stop)
  * Modifier keys (shift, alt, etc.)
  * Custom editing macros
- Design custom PCB for more compact form factor
- Add battery charging circuit
- Explore using magnetic encoders for better tactile feedback

### Software Improvements
- Integrate simultaneous keyboard and mouse functionality:
  * Encoder-based timeline scrubbing
  * Fine control with modifier keys
  * Mouse buttons for editing operations
  * Customizable jog speeds
- Add OLED display support:
  * Timeline position/zoom level
  * Current jog speed/sensitivity
  * Active modifiers/functions
  * Battery and connection status
- Add OTA (Over-The-Air) firmware updates
- Implement configuration storage in flash memory
- Add diagnostic mode for troubleshooting
- Improve BLE reconnection reliability
- Add support for multiple device pairing

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.
