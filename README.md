# ESP32-C6-BLE-Serial-Bridge
A versatile, handheld tool for accessing a serial port via Bluetooth Low Energy (BLE), featuring on-the-fly baud rate selection and real-time status display. Perfect for console management, device configuration, and serial communication on the go!
![alt text](https://github.com/noname1020/ESP32-C6-BLE-Serial-Bridge/blob/main/PXL_20250927_184047986.MP.jpg)
![alt text](https://github.com/noname1020/ESP32-C6-BLE-Serial-Bridge-adapter/blob/main/PXL_20250927_184030924.MP.jpg).
✨ Features
Wireless Serial Access: Connect to the serial port of devices like firewalls, routers, or motor controllers via BLE using a smartphone or tablet.
Selectable Baud Rates: Easily toggle between common baud rates using a single button:
9600
19200
38400
57600
115200
Real-time Status Display: A 128x32 I2C OLED display shows:
Current Baud Rate
BLE Connection Status
Tx/Rx (Transmit/Receive) data activity indicators
Wide Application: Ideal for:
Installing/Managing console-mode firewalls (pfSense, OPNsense)
Configuring embedded systems (e.g., servo motor controllers with PID settings)
General serial debugging and communication.

🛠️ Hardware Components
The following components are required to build this project:

Component	Description
Microcontroller:	Seeeduino XIAO ESP32-C6
Serial Converter:	RS232 TTL Converter Module (3.3V/5V compatible)
Display:	128x32 I2C OLED Display
Interface:	Momentary Push Button (for baud rate selection)

🔌 Wiring and Setup
The code is designed to be self-explanatory for the wiring and pin assignments. Please refer to the main.ino
for detailed pinouts connecting the XIAO ESP32-C6 to the RS232 TTL converter, OLED display, and baud rate selection button.
OLED (I2C): Connects to the designated SDA/SCL pins on the ESP32-C6.
RS232 Converter (TTL): Connects to the designated UART TX/RX pins on the ESP32-C6.
Selection Button: Connects to a designated digital input pin on the ESP32-C6.

💻 Usage
Flash the Code: Upload the provided code to your XIAO ESP32-C6.
Initial Power-Up: The device will power on and display the default baud rate 115200.
Select Baud Rate: Press the selection button to cycle through the available baud rates until you match the setting of your target device.
Connect to Target: Plug the RS232 converter into your target device's serial port.
Pair via BLE: On your smartphone or tablet, use a Serial Bluetooth Terminal application to scan for and connect to the device's advertised BLE service (The name is set in the code).
Communicate: Once paired, you can send and receive serial data wirelessly! The OLED will update to show the connected status.

🤖 AI-Powered Development
This entire codebase was collaboratively completed using Google Gemini AI and ChatGPT AI. The code is designed to be well-structured and commented to clearly explain its functionality, state machine, and hardware interaction logic.

