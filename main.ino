#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Nordic UART Service (NUS) UUIDs
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Pin Definitions
// Note: You might need to adjust these pins for your specific ESP32 board
#define UART_RX_PIN D7 // Commonly used as RX2 on many ESP32 boards
#define UART_TX_PIN D6 // Commonly used as TX2 on many ESP32 boards
#define BUTTON_PIN  D2  // A safe GPIO pin for input

// Baud rates for Serial1
int baudRates[] = {9600, 19200, 38400, 57600, 115200};
int baudIndex = 4; // Start with 115200

// BLE Objects
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// --- Circular Buffer for BLE to Serial Data ---
#define QUEUE_SIZE 2048 // Increased queue size for better buffering
uint8_t bleToSerialQueue[QUEUE_SIZE];
volatile size_t bleQueueStart = 0;
volatile size_t bleQueueEnd = 0;

// Activity flags for display
volatile bool txActivity = false;
volatile bool rxActivity = false;

// Timers
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 200;

// --- Display Helper Function ---
void showStatus() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    // Connection Status
    if (deviceConnected) {
        display.print("BLE: CONNECTED");
    } else {
        display.print("BLE: STANDBY");
    }
    
    display.setCursor(0, 10);
    // TX/RX Activity Indicators
    display.print(txActivity ? "TX > " : "TX - ");
    display.print(rxActivity ? "RX < " : "RX - ");

    display.setCursor(0, 22);
    display.setTextSize(1);
    display.print("BAUD: ");
    display.print(baudRates[baudIndex]);

    display.display();

    // Reset activity flags after displaying them
    txActivity = false;
    rxActivity = false;
}

// --- BLE Server Callbacks ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
      //  Serial.println("Client connected");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
       // Serial.println("Client disconnected");
    }
};

// --- BLE Characteristic Callbacks ---
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            rxActivity = true; // Set RX activity flag
            // Push received data to the circular buffer
            for (size_t i = 0; i < rxValue.length(); i++) {
                size_t nextEnd = (bleQueueEnd + 1) % QUEUE_SIZE;
                if (nextEnd != bleQueueStart) { // Avoid buffer overflow
                    bleToSerialQueue[bleQueueEnd] = rxValue[i];
                    bleQueueEnd = nextEnd;
                }
            }
        }
    }
};

// --- Baud Rate Change Function ---
void changeBaudRate() {
    baudIndex = (baudIndex + 1) % (sizeof(baudRates)/sizeof(baudRates[0]));
    Serial1.flush(); // Wait for outgoing data to be sent
    Serial1.updateBaudRate(baudRates[baudIndex]);
    //Serial.print("Switched to baud rate: ");
    //Serial.println(baudRates[baudIndex]);
    showStatus(); // Update display immediately
}

// --- Setup ---
void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    //Serial.begin(115200);
    Serial1.begin(baudRates[baudIndex], SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    // Initialize OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
       // Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }
    display.clearDisplay();
    display.display();
    showStatus();

    // Initialize BLE
    BLEDevice::init("ESP32 UART Bridge");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create TX Characteristic (Server to Client)
    pTxCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_TX,
                                         BLECharacteristic::PROPERTY_NOTIFY
                                     );
    pTxCharacteristic->addDescriptor(new BLE2902());

    // Create RX Characteristic (Client to Server)
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                                               CHARACTERISTIC_UUID_RX,
                                               BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
                                           );
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();
    pServer->getAdvertising()->start();
    //Serial.println("Waiting for a client connection...");
}

// --- Main Loop ---
void loop() {
    unsigned long now = millis();
    static bool lastButtonState = HIGH;
    static unsigned long lastButtonCheck = 0;

    // --- Handle BLE Connection State ---
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        showStatus();
    }
    if (!deviceConnected && oldDeviceConnected) {
        pServer->startAdvertising();
        //Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
        showStatus();
    }

    // --- Non-blocking Button Check for Baud Rate ---
    if (now - lastButtonCheck > 50) { // Debounce delay
        lastButtonCheck = now;
        bool buttonState = digitalRead(BUTTON_PIN);
        if (lastButtonState == HIGH && buttonState == LOW) {
            changeBaudRate();
        }
        lastButtonState = buttonState;
    }

    // --- Forward data from BLE queue to Serial1 ---
    if (bleQueueStart != bleQueueEnd) {
        // Only write if there's space in the Serial TX buffer
        while (Serial1.availableForWrite() && bleQueueStart != bleQueueEnd) {
            Serial1.write(bleToSerialQueue[bleQueueStart]);
            bleQueueStart = (bleQueueStart + 1) % QUEUE_SIZE;
        }
    }

    // --- Forward data from Serial1 to BLE ---
    if (Serial1.available()) {
        txActivity = true; // Set TX activity flag
        // Use a temporary buffer to read all available bytes from Serial1
        uint8_t tempBuffer[512]; // Max BLE packet size is usually around 512
        size_t len = 0;
        while(Serial1.available() && len < sizeof(tempBuffer)) {
            tempBuffer[len++] = Serial1.read();
        }

        if (deviceConnected && len > 0) {
            pTxCharacteristic->setValue(tempBuffer, len);
            pTxCharacteristic->notify();
        }
    }

    // --- Update OLED Display Periodically ---
    if (now - lastDisplayUpdate > DISPLAY_INTERVAL) {
        lastDisplayUpdate = now;
        if (txActivity || rxActivity || (deviceConnected != oldDeviceConnected)) {
           showStatus();
        }
    }
}


