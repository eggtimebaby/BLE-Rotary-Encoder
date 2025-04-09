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

// First Rotary Encoder pins (Left/Right arrows + Enter)
#define PIN_ENC1_SW  0  // Encoder 1 button
#define PIN_ENC1_DT  1  // Encoder 1 DT
#define PIN_ENC1_CLK 2  // Encoder 1 CLK

// Second Rotary Encoder pins (Up/Down arrows + Command)
#define PIN_ENC2_SW  8  // Encoder 2 button
#define PIN_ENC2_DT  7  // Encoder 2 DT
#define PIN_ENC2_CLK 6  // Encoder 2 CLK

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

// Button state management
enum ButtonState {
  BUTTON_UP,
  BUTTON_DEBOUNCING_DOWN,
  BUTTON_DOWN,
  BUTTON_DEBOUNCING_UP
};

// Encoder state management
struct EncoderState {
  volatile int position;
  int lastReportedPosition;
  unsigned long lastEvent;
  volatile ButtonState buttonState;
  volatile unsigned long buttonStateChangeTime;
  bool buttonEventProcessed;
};

// Timing constants
const unsigned long encoderDebounceTime = 5;   // Encoder debounce time in milliseconds
const unsigned long buttonDebounceTime = 50;   // Button debounce time in milliseconds

// State for each encoder
EncoderState encoder1 = {0, 0, 0, BUTTON_UP, 0, true};  // Left/Right + Enter
EncoderState encoder2 = {0, 0, 0, BUTTON_UP, 0, true};  // Up/Down + Command

// Function prototypes
void updateLED();
void handleEncoder1Change();
void handleEncoder2Change();
void handleButton1Event();
void handleButton2Event();
void updateConnectionState();

// ISR for first encoder (Left/Right)
void IRAM_ATTR encoder1ISR() {
  // Read the current state of the encoder pins
  bool pinA = digitalRead(PIN_ENC1_CLK);
  bool pinB = digitalRead(PIN_ENC1_DT);
  
  // Simple debounce - ignore events that happen too quickly
  unsigned long now = millis();
  if (now - encoder1.lastEvent < encoderDebounceTime) {
    return;
  }
  
  // Determine direction based on the state of both pins
  static uint8_t old_AB = 0;
  uint8_t current_AB = (pinA << 1) | pinB;
  
  // Update position based on the rotation direction
  if ((old_AB == 0b00 && current_AB == 0b01) ||
      (old_AB == 0b01 && current_AB == 0b11) ||
      (old_AB == 0b11 && current_AB == 0b10) ||
      (old_AB == 0b10 && current_AB == 0b00)) {
    // Clockwise rotation
    encoder1.position++;
    encoder1.lastEvent = now;
  } else if ((old_AB == 0b00 && current_AB == 0b10) ||
             (old_AB == 0b10 && current_AB == 0b11) ||
             (old_AB == 0b11 && current_AB == 0b01) ||
             (old_AB == 0b01 && current_AB == 0b00)) {
    // Counter-clockwise rotation
    encoder1.position--;
    encoder1.lastEvent = now;
  }
  
  old_AB = current_AB;
}

// ISR for second encoder (Up/Down)
void IRAM_ATTR encoder2ISR() {
  // Read the current state of the encoder pins
  bool pinA = digitalRead(PIN_ENC2_CLK);
  bool pinB = digitalRead(PIN_ENC2_DT);
  
  // Simple debounce - ignore events that happen too quickly
  unsigned long now = millis();
  if (now - encoder2.lastEvent < encoderDebounceTime) {
    return;
  }
  
  // Determine direction based on the state of both pins
  static uint8_t old_AB = 0;
  uint8_t current_AB = (pinA << 1) | pinB;
  
  // Update position based on the rotation direction
  if ((old_AB == 0b00 && current_AB == 0b01) ||
      (old_AB == 0b01 && current_AB == 0b11) ||
      (old_AB == 0b11 && current_AB == 0b10) ||
      (old_AB == 0b10 && current_AB == 0b00)) {
    // Clockwise rotation
    encoder2.position++;
    encoder2.lastEvent = now;
  } else if ((old_AB == 0b00 && current_AB == 0b10) ||
             (old_AB == 0b10 && current_AB == 0b11) ||
             (old_AB == 0b11 && current_AB == 0b01) ||
             (old_AB == 0b01 && current_AB == 0b00)) {
    // Counter-clockwise rotation
    encoder2.position--;
    encoder2.lastEvent = now;
  }
  
  old_AB = current_AB;
}

// ISR for first encoder button (Enter)
void IRAM_ATTR button1ISR() {
  unsigned long now = millis();
  
  // State machine for button debouncing
  switch (encoder1.buttonState) {
    case BUTTON_UP:
      if (digitalRead(PIN_ENC1_SW) == LOW) {
        encoder1.buttonState = BUTTON_DEBOUNCING_DOWN;
        encoder1.buttonStateChangeTime = now;
      }
      break;
      
    case BUTTON_DEBOUNCING_DOWN:
      // We don't handle this in the ISR, it's handled in the main loop
      break;
      
    case BUTTON_DOWN:
      if (digitalRead(PIN_ENC1_SW) == HIGH) {
        encoder1.buttonState = BUTTON_DEBOUNCING_UP;
        encoder1.buttonStateChangeTime = now;
      }
      break;
      
    case BUTTON_DEBOUNCING_UP:
      // We don't handle this in the ISR, it's handled in the main loop
      break;
  }
}

// ISR for second encoder button (Command)
void IRAM_ATTR button2ISR() {
  unsigned long now = millis();
  
  // State machine for button debouncing
  switch (encoder2.buttonState) {
    case BUTTON_UP:
      if (digitalRead(PIN_ENC2_SW) == LOW) {
        encoder2.buttonState = BUTTON_DEBOUNCING_DOWN;
        encoder2.buttonStateChangeTime = now;
      }
      break;
      
    case BUTTON_DEBOUNCING_DOWN:
      // We don't handle this in the ISR, it's handled in the main loop
      break;
      
    case BUTTON_DOWN:
      if (digitalRead(PIN_ENC2_SW) == HIGH) {
        encoder2.buttonState = BUTTON_DEBOUNCING_UP;
        encoder2.buttonStateChangeTime = now;
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
  Serial.println("Starting Enhanced BLE Dual Rotary Encoder Keyboard");
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  leds[0] = colorDisconnected;
  FastLED.show();
  
  // Initialize first rotary encoder pins with pull-up resistors
  pinMode(PIN_ENC1_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC1_DT, INPUT_PULLUP);
  pinMode(PIN_ENC1_SW, INPUT_PULLUP);
  
  // Initialize second rotary encoder pins with pull-up resistors
  pinMode(PIN_ENC2_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC2_DT, INPUT_PULLUP);
  pinMode(PIN_ENC2_SW, INPUT_PULLUP);
  
  // Attach interrupts for first rotary encoder and button
  attachInterrupt(digitalPinToInterrupt(PIN_ENC1_CLK), encoder1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC1_DT), encoder1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC1_SW), button1ISR, CHANGE);
  
  // Attach interrupts for second rotary encoder and button
  attachInterrupt(digitalPinToInterrupt(PIN_ENC2_CLK), encoder2ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC2_DT), encoder2ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC2_SW), button2ISR, CHANGE);
  
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
    handleEncoder1Change();
    handleEncoder2Change();
  }
  
  // Process button state machines
  handleButton1Event();
  handleButton2Event();
  
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

void handleEncoder1Change() {
  // Check if encoder position has changed
  if (encoder1.position != encoder1.lastReportedPosition) {
    // Show activity on LED
    leds[0] = colorActivity;
    FastLED.show();
    
    // Determine direction and send appropriate key
    if (encoder1.position > encoder1.lastReportedPosition) {
      Serial.println("Sending RIGHT key");
      bleKeyboard.write(KEY_RIGHT_ARROW);
    } else {
      Serial.println("Sending LEFT key");
      bleKeyboard.write(KEY_LEFT_ARROW);
    }
    
    // Update last reported position
    encoder1.lastReportedPosition = encoder1.position;
    
    // Short delay to prevent multiple key presses
    delay(10);
    
    // Reset LED to connected state
    leds[0] = colorConnected;
    FastLED.show();
  }
}

void handleEncoder2Change() {
  // Check if encoder position has changed
  if (encoder2.position != encoder2.lastReportedPosition) {
    // Show activity on LED
    leds[0] = colorActivity;
    FastLED.show();
    
    // Determine direction and send appropriate key
    if (encoder2.position > encoder2.lastReportedPosition) {
      Serial.println("Sending UP key");
      bleKeyboard.write(KEY_UP_ARROW);
    } else {
      Serial.println("Sending DOWN key");
      bleKeyboard.write(KEY_DOWN_ARROW);
    }
    
    // Update last reported position
    encoder2.lastReportedPosition = encoder2.position;
    
    // Short delay to prevent multiple key presses
    delay(10);
    
    // Reset LED to connected state
    leds[0] = colorConnected;
    FastLED.show();
  }
}

void handleButton1Event() {
  unsigned long now = millis();
  
  // Button state machine
  switch (encoder1.buttonState) {
    case BUTTON_DEBOUNCING_DOWN:
      if (now - encoder1.buttonStateChangeTime > buttonDebounceTime) {
        if (digitalRead(PIN_ENC1_SW) == LOW) {
          encoder1.buttonState = BUTTON_DOWN;
          encoder1.buttonEventProcessed = false;
          Serial.println("Button 1 pressed (debounced)");
        } else {
          encoder1.buttonState = BUTTON_UP;  // It was noise
        }
      }
      break;
      
    case BUTTON_DEBOUNCING_UP:
      if (now - encoder1.buttonStateChangeTime > buttonDebounceTime) {
        if (digitalRead(PIN_ENC1_SW) == HIGH) {
          encoder1.buttonState = BUTTON_UP;
          Serial.println("Button 1 released (debounced)");
        } else {
          encoder1.buttonState = BUTTON_DOWN;  // It was noise
        }
      }
      break;
      
    case BUTTON_DOWN:
      // Process button press if connected and not yet processed
      if (connectionState == CONNECTED && !encoder1.buttonEventProcessed) {
        // Show activity on LED
        leds[0] = colorActivity;
        FastLED.show();
        
        Serial.println("Sending ENTER key");
        bleKeyboard.write(KEY_RETURN);
        
        // Mark as processed
        encoder1.buttonEventProcessed = true;
        
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

void handleButton2Event() {
  unsigned long now = millis();
  
  // Button state machine
  switch (encoder2.buttonState) {
    case BUTTON_DEBOUNCING_DOWN:
      if (now - encoder2.buttonStateChangeTime > buttonDebounceTime) {
        if (digitalRead(PIN_ENC2_SW) == LOW) {
          encoder2.buttonState = BUTTON_DOWN;
          encoder2.buttonEventProcessed = false;
          Serial.println("Button 2 pressed (debounced)");
        } else {
          encoder2.buttonState = BUTTON_UP;  // It was noise
        }
      }
      break;
      
    case BUTTON_DEBOUNCING_UP:
      if (now - encoder2.buttonStateChangeTime > buttonDebounceTime) {
        if (digitalRead(PIN_ENC2_SW) == HIGH) {
          encoder2.buttonState = BUTTON_UP;
          Serial.println("Button 2 released (debounced)");
        } else {
          encoder2.buttonState = BUTTON_DOWN;  // It was noise
        }
      }
      break;
      
    case BUTTON_DOWN:
      // Process button press if connected and not yet processed
      if (connectionState == CONNECTED && !encoder2.buttonEventProcessed) {
        // Show activity on LED
        leds[0] = colorActivity;
        FastLED.show();
        
        Serial.println("Sending Command key");
        bleKeyboard.write(KEY_LEFT_GUI);  // Command key on Mac
        
        // Mark as processed
        encoder2.buttonEventProcessed = true;
        
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
