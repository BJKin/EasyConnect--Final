# EasyConnect Hardware Repository Overview
```
EasyConnect-Final/                            
├── Embedded/                           # ESP32-C6 embedded code
│   ├── include/                        # C++ header files
│   ├── src/                            # Main source code
│   │   ├── BLE.cpp                     # Bluetooth Low Energy
│   │   ├── ECE140_MQTT.cpp            # MQTT communication
│   │   ├── ECE140_WIFI.cpp            # WiFi connection
│   │   ├── Handshake.cpp              # Handshake detection
│   │   └── main.cpp                   # Main script
│   └── pre_extra_script.py           # Build scripts
├── MLHandshakeDataCollection/          # Data collection
│   └── MLHandshakeDataCollection      # Arduino IDE data collector
├── TensorFlow/                         # Machine learning pipeline
│   ├── Data/                          # Organized datasets
│   ├── ModelAnalysis/                 # Model performance graphs
│   ├── Models/                        # Trained models
│   ├── Working/                       # Working models
│   ├── dataCollection.py             # Data collection 
│   └── train.py                      # Model training 
└── README.MD                          # Documentation
```

## Directory Overview

### Embedded/
Contains the complete ESP32-C6 embedded software stack.

**Key Components:**
- **BLE.cpp**: Manages Bluetooth Low Energy for device-to-device communication
- **ECE140_MQTT.cpp**: Handles MQTT messaging for device-to-server communication
- **ECE140_WIFI.cpp**: Manages WiFi connectivity (enterprise and standard network support)
- **Handshake.cpp**: Implements handshake detection and IMU data processing algorithms
- **main.cpp**: Main execution loop

### TensorFlow/
Contains all of the machine learning software for training handshake detection models.

**Structure:**
- **Data/**: Categorized raw sensor data
- **Models/**: Trained neural networks as .tflite files
- **ModelAnalysis/**: Performance metric graphs
- **Working/**: Working neural networks as C++ header files (needed for embedded deployment)

**Scripts:**
- **train.py**: Training pipeline for handshake classification models
- **dataCollection.py**: Data collection

### MLHandshakeDataCollection/
Contains the Arduino IDE script (.ino file) used to collect data.


# EasyConnect Embedded Software Setup

## Prerequisites

### Development Environment
1. **Install Visual Studio Code**
   - Download from: https://code.visualstudio.com/

2. **Install PlatformIO Extension**
   - Installation guide: https://platformio.org/install/ide?install=vscode
   - Open VS Code and install the PlatformIO extension from the marketplace

### Repository Setup
3. **Clone the Repository**
   ```bash
   git clone https://github.com/BJKin/EasyConnect--Final
   ```

## MQTT Broker Configuration

### HiveMQ Setup
1. **Create HiveMQ Account**
   - Visit: https://www.hivemq.com/
   - Sign up for the **Starter tier**

2. **Create New Cluster**
   - Create a new cluster and assign it a descriptive name
   - Navigate to your cluster by clicking on its name

3. **Configure Cluster Settings**
   - **Overview Section**: Note the cluster URL for later use
   - **Access Management Tab**:
     - Create new access credentials
     - Record the username and password securely

4. **Test Connection**
   - Navigate to the **Web Client** tab
   - Connect using your previously created credentials
   - Subscribe to the following topics:
     - `device/#`
     - `event/#`
   - Keep this window open for testing after device setup

## Project Configuration

### VS Code Setup
1. **Open Project**
   - Launch VS Code
   - Select the PlatformIO extension
   - Click "Open Project"
   - Navigate to the cloned repository and open the `Embedded` folder

2. **Environment Configuration**
   - Create a `.env` file in the root directory
   - Add the following variables:

   ```env
   UCSD_USERNAME=your_ucsd_username
   UCSD_PASSWORD=your_ucsd_password
   ENTERPRISE_WIFI_SSID=your_enterprise_wifi_ssid
   WIFI_SSID=your_wifi_ssid
   NON_ENTERPRISE_WIFI_SSID=your_non_enterprise_wifi_ssid
   MQTT_SERVER=your_hivemq_cluster_url
   MQTT_PORT=8883
   MQTT_USERNAME=your_mqtt_username
   MQTT_PASSWORD=your_mqtt_password
   EVENT_ID=E_xx
   BLE_IDENTIFIER=your_6_digit_alphanumeric_string
   ```

   **Configuration Notes:**
   - `EVENT_ID`: Must follow format `E_xx` (e.g., `E_01`, `E_02`)
   - `BLE_IDENTIFIER`: Any 6-digit alphanumeric string
   - `MQTT_SERVER`: Use the cluster URL from your HiveMQ overview

3. **WiFi Configuration**
   - Locate lines 95/96 in the main.cpp file under the src folder
   - Comment/uncomment the appropriate WiFi connection lines based on your network type:
     - Enterprise WiFi: Uncomment enterprise connection code/ comment out standard WiFi code
     - Standard WiFi: Uncomment standard connection code/ comment out Enterprise WiFi code

## Deployment

### Flash Device
1. **Upload Code**
   - Connect your ESP32-C6 device via USB-C
   - Click the right arrow button (→) at the top right of the IDE window
   - Wait for the upload process to complete

2. **Monitor Device**
   - Click the plug button at the top right of the IDE window to open Serial Monitor
   - Observe the output for:
     - I²C connections log
     - WiFi connection status
     - MQTT broker connection

3. **Verify Connection**
   - Return to the HiveMQ Web Client tab
   - You should see confirmation messages indicating the device has connected to the broker

## Device Control

### MQTT Endpoints

#### Subscriptions (Send commands to device)

**Device-Specific Commands:**
- Topic: `device/{clientId}/#`
  - `/assignment` - Assign device to ticket ID (format: `T_xxxxxx`)
  - `/reboot` - Reboot the device
  - `/reassignment` - Change ticket ID associated with device
  - `/resetNFC` - Erase and rewrite NFC chip memory

**Event-Wide Commands:**
- Topic: `event/{eventId}/#`
  - `/reboot` - Reboot all devices at an event
  - `/profile_swap/#` - Provide haptic feedback to devices during profile swap

#### Publications (Device sends data)

**Device Registration:**
- `event/{eventId}/available_devices/{clientId}` - Store device info in database

**Transaction Logging:**
- `device/{clientId}/receipt` - Publish receipt after MQTT transactions

**Profile Management:**
- `event/{eventId}/profile_swap` - Handle profile swaps between devices after handshake

## Testing

1. **Device Connection Test**
   - Verify device appears in HiveMQ Web Client
   - Send test commands via MQTT topics
   - Monitor serial output for responses

2. **Functionality Test (single device)**
   - Assign device a ticket ID using the HiveMQ Web Client
   - Perform a handshake with the device 
   - Confirm a handshake was detected in the HiveMQ Web Client

3. **Functionality Test (two devices)**
   - Assign each device a unique ticket ID using the HiveMQ Web Client
   - Perform a handshake between two devices
   - Confirm that a profile swap occurred in the HiveMQ Web Client

## Troubleshooting

### Common Issues
- **Device not connecting**: Check WiFi credentials and network configuration
- **MQTT connection failed**: Verify HiveMQ credentials and cluster URL
- **Upload failed**: Check USB connection and ensure correct board is selected

### Debug Steps
1. Check serial monitor output for error messages
2. Verify all environment variables are correctly set
3. Ensure proper hardware connections as per wiring instructions
4. Test MQTT broker connectivity using Web Client