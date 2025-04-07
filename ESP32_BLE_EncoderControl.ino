/**
 * ESP32-C3 BLE Keyboard with Enhanced Rotary Encoder
 * 
 * This sketch turns the ESP32-C3 into a Bluetooth LE keyboard that uses a rotary encoder
 * to send left and right arrow key presses, and the encoder button to send Enter key.
 * Features improved debouncing and robust BLE connection handling.
 * 
 * Hardware:
 * - ESP32-C3
 * - Rotary Encoder connected to:
 *   - SW (Button): GPIO 0
 *   - DT: GPIO 1
 *   - CLK: GPIO 2
 * - Status LED on GPIO 10 (using FastLED)
 * 
 * Dependencies:
 * - ESP32-BLE-Keyboard library
 * - FastLED library
 */

#include <BleKeyboard.h>
#include <FastLED.h>

// Rotary Encoder pins
#define PIN_ENC_SW  0  // Encoder button
#define PIN_ENC_DT  1  // Encoder DT
#define PIN_ENC_CLK 2  // Encoder CLK

// LED Configuration
#define LED_PIN     10
#define NUM_LEDS    1
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

// BLE Configuration
#define DEVICE_NAME "ESP32-C3 Encoder"
#define MANUFACTURER "Espressif"
#define RECONNECT_DELAY 5000  // Time between reconnection attempts (ms)

// Custom BLE Keyboard class with callbacks
class MyBleKeyboard : public BleKeyboard {
public:
  MyBleKeyboard(String deviceName = "ESP32 BLE Keyboard", String deviceManufacturer = "Espressif", uint8_t batteryLevel = 100) 
    : BleKeyboard(deviceName, deviceManufacturer, batteryLevel) {}
    
protected:
  virtual void onConnect(BLEServer* pServer) override {
    BleKeyboard::onConnect(pServer);
    Serial.println("BLE CONNECTED callback triggered!");
  }
  
  virtual void onDisconnect(BLEServer* pServer) override {
    BleKeyboard::onDisconnect(pServer);
    Serial.println("BLE DISCONNECTED callback triggered!");
  }
};

// Create BLE Keyboard instance
MyBleKeyboard bleKeyboard(DEVICE_NAME, MANUFACTURER, 100);

// Create LED array
CRGB leds[NUM_LEDS];

// LED status colors
CRGB colorDisconnected = CRGB(50, 0, 0);    // Red when disconnected
CRGB colorConnecting = CRGB(50, 20, 0);     // Orange when trying to connect
CRGB colorConnected = CRGB(0, 50, 0);       // Green when connected
CRGB colorActivity = CRGB(0, 0, 50);        // Blue during activity

// Connection state management
enum ConnectionState {
  DISCONNECTED,
  CONNECTING,
  CONNECTED
};

ConnectionState connectionState = DISCONNECTED;
unsigned long lastConnectionAttempt = 0;
bool wasConnected = false;

// Rotary encoder state management
volatile int encoderPos = 0;
int lastReportedPos = 0;
unsigned long lastEncoderEvent = 0;
const unsigned long encoderDebounceTime = 5;  // Encoder debounce time in milliseconds

// Button state management
enum ButtonState {
  BUTTON_UP,
  BUTTON_DEBOUNCING_DOWN,
  BUTTON_DOWN,
  BUTTON_DEBOUNCING_UP
};

volatile ButtonState buttonState = BUTTON_UP;
volatile unsigned long buttonStateChangeTime = 0;
const unsigned long buttonDebounceTime = 50;  // Button debounce time in milliseconds
bool buttonEventProcessed = true;

// Function prototypes
void updateLED();
void handleEncoderChange();
void handleButtonEvent();
void updateConnectionState();

void IRAM_ATTR encoderISR() {
  // Read the current state of the encoder pins
  bool pinA = digitalRead(PIN_ENC_CLK);
  bool pinB = digitalRead(PIN_ENC_DT);
  
  // Simple debounce - ignore events that happen too quickly
  unsigned long now = millis();
  if (now - lastEncoderEvent < encoderDebounceTime) {
    return;
  }
  
  // Determine direction based on the state of both pins
  // This is a simplified approach - a full state machine would be more robust
  static uint8_t old_AB = 0;
  uint8_t current_AB = (pinA << 1) | pinB;
  
  // Update position based on the rotation direction
  if ((old_AB == 0b00 && current_AB == 0b01) ||
      (old_AB == 0b01 && current_AB == 0b11) ||
      (old_AB == 0b11 && current_AB == 0b10) ||
      (old_AB == 0b10 && current_AB == 0b00)) {
    // Clockwise rotation
    encoderPos++;
    lastEncoderEvent = now;
  } else if ((old_AB == 0b00 && current_AB == 0b10) ||
             (old_AB == 0b10 && current_AB == 0b11) ||
             (old_AB == 0b11 && current_AB == 0b01) ||
             (old_AB == 0b01 && current_AB == 0b00)) {
    // Counter-clockwise rotation
    encoderPos--;
    lastEncoderEvent = now;
  }
  
  old_AB = current_AB;
}

void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  
  // State machine for button debouncing
  switch (buttonState) {
    case BUTTON_UP:
      if (digitalRead(PIN_ENC_SW) == LOW) {
        buttonState = BUTTON_DEBOUNCING_DOWN;
        buttonStateChangeTime = now;
      }
      break;
      
    case BUTTON_DEBOUNCING_DOWN:
      // We don't handle this in the ISR, it's handled in the main loop
      break;
      
    case BUTTON_DOWN:
      if (digitalRead(PIN_ENC_SW) == HIGH) {
        buttonState = BUTTON_DEBOUNCING_UP;
        buttonStateChangeTime = now;
      }
      break;
      
    case BUTTON_DEBOUNCING_UP:
      // We don't handle this in the ISR, it's handled in the main loop
      break;
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Starting Enhanced BLE Rotary Encoder Keyboard");
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  leds[0] = colorDisconnected;
  FastLED.show();
  
  // Initialize rotary encoder pins with pull-up resistors
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  
  // Attach interrupts for rotary encoder and button
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), buttonISR, CHANGE);
  
  // Start BLE Keyboard
  bleKeyboard.begin();
  connectionState = CONNECTING;
  lastConnectionAttempt = millis();
  
  Serial.println("Setup complete, waiting for BLE connection...");
}

void loop() {
  // Update connection state
  updateConnectionState();
  
  // Update LED based on current state
  updateLED();
  
  // Process encoder changes if connected
  if (connectionState == CONNECTED) {
    handleEncoderChange();
  }
  
  // Process button state machine
  handleButtonEvent();
  
  // Small delay to reduce CPU usage
  delay(5);
}

void updateConnectionState() {
  bool isConnected = bleKeyboard.isConnected();
  
  // Detect connection changes
  if (isConnected && !wasConnected) {
    Serial.println("BLE connected!");
    connectionState = CONNECTED;
    wasConnected = true;
  } else if (!isConnected && wasConnected) {
    Serial.println("BLE disconnected!");
    connectionState = DISCONNECTED;
    wasConnected = false;
    lastConnectionAttempt = millis();
  }
  
  // Handle reconnection attempts
  if (connectionState == DISCONNECTED) {
    unsigned long now = millis();
    if (now - lastConnectionAttempt > RECONNECT_DELAY) {
      Serial.println("Attempting to reconnect BLE...");
      connectionState = CONNECTING;
      // No explicit reconnect method in the library, but we can update the state
      lastConnectionAttempt = now;
    }
  } else if (connectionState == CONNECTING && isConnected) {
    connectionState = CONNECTED;
  }
}

void updateLED() {
  // Normal LED behavior based on connection state
  switch (connectionState) {
    case DISCONNECTED:
      leds[0] = colorDisconnected;
      break;
    case CONNECTING:
      // Blink the LED when connecting
      if ((millis() / 500) % 2 == 0) {
        leds[0] = colorConnecting;
      } else {
        leds[0] = CRGB::Black;
      }
      break;
    case CONNECTED:
      leds[0] = colorConnected;
      break;
  }
  FastLED.show();
}

void handleEncoderChange() {
  // Check if encoder position has changed
  if (encoderPos != lastReportedPos) {
    // Show activity on LED
    leds[0] = colorActivity;
    FastLED.show();
    
    // Determine direction and send appropriate key
    if (encoderPos > lastReportedPos) {
      Serial.println("Sending RIGHT key");
      bleKeyboard.write(KEY_RIGHT_ARROW);
    } else {
      Serial.println("Sending LEFT key");
      bleKeyboard.write(KEY_LEFT_ARROW);
    }
    
    // Update last reported position
    lastReportedPos = encoderPos;
    
    // Short delay to prevent multiple key presses
    delay(10);
    
    // Reset LED to connected state
    leds[0] = colorConnected;
    FastLED.show();
  }
}

void handleButtonEvent() {
  unsigned long now = millis();
  
  // Button state machine
  switch (buttonState) {
    case BUTTON_DEBOUNCING_DOWN:
      if (now - buttonStateChangeTime > buttonDebounceTime) {
        if (digitalRead(PIN_ENC_SW) == LOW) {
          buttonState = BUTTON_DOWN;
          buttonEventProcessed = false;
          Serial.println("Button pressed (debounced)");
        } else {
          buttonState = BUTTON_UP;  // It was noise
        }
      }
      break;
      
    case BUTTON_DEBOUNCING_UP:
      if (now - buttonStateChangeTime > buttonDebounceTime) {
        if (digitalRead(PIN_ENC_SW) == HIGH) {
          buttonState = BUTTON_UP;
          Serial.println("Button released (debounced)");
        } else {
          buttonState = BUTTON_DOWN;  // It was noise
        }
      }
      break;
      
    case BUTTON_DOWN:
      // Process button press if connected and not yet processed
      if (connectionState == CONNECTED && !buttonEventProcessed) {
        // Show activity on LED
        leds[0] = colorActivity;
        FastLED.show();
        
        Serial.println("Sending ENTER key");
        bleKeyboard.write(KEY_RETURN);
        
        // Mark as processed
        buttonEventProcessed = true;
        
        // Short delay to prevent multiple key presses
        delay(10);
        
        // Reset LED to connected state
        leds[0] = colorConnected;
        FastLED.show();
      }
      break;
      
    case BUTTON_UP:
      // Nothing to do in this state
      break;
  }
}
