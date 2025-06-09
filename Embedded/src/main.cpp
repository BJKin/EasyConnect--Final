#include "ECE140_WIFI.h"
#include "ECE140_MQTT.h"
#include "BLE.h"
#include "Handshake.h"
#include <Adafruit_BNO055.h>
#include <SparkFun_ST25DV64KC_Arduino_Library.h>
#include <algorithm>

// MQTT Credentials
const char* broker = MQTT_SERVER;
const char* broker_username = MQTT_USERNAME;
const char* broker_password = MQTT_PASSWORD;

// WiFi credentials (UCSD proteceted)
const char* ucsdUsername = UCSD_USERNAME;
const char* ucsdPassword = UCSD_PASSWORD;
const char* ucsdWifi = ENTERPRISE_WIFI_SSID;


// WiFi credentials (Home network)
const char* wifiSsid = WIFI_SSID;
const char* nonEnterpriseWifiPassword = NON_ENTERPRISE_WIFI_PASSWORD;

// Instantiate to NFC
SFE_ST25DV64KC_NDEF tag;

//Instantiate WiFi/MQTT/BLE
ECE140_WIFI wifi;
ECE140_MQTT mqtt;
BLE ble;

// Device specific variables
String ticketId;
String eventId;
String macAddress;

// State variables
bool assigned = false;
bool tagWritten = false;
bool handshakeDetected = false;
bool deviceFound = false;
bool BLEstarted = false;
bool profileExchanged = false;

// BLE
String foundId;
String prevId;
std::vector<String> detectedTickets;
unsigned long collectionStart = 0;
static const unsigned long COLLECTION_TIME = 1000; 

// Buzzer
const int pwmBitResolution = 8;
const int pwmFrequency = 5000;
const int MOTOR_PIN = 1;
unsigned long buzzTime = 0;
unsigned long buzzDuration = 0;

// IMU/TensorFlow
Handshake handshake;
unsigned long timeDetected = 0;

// Haptic feedback
void setupFeedback(){
    ledcAttach(MOTOR_PIN, pwmFrequency, pwmBitResolution);
}

void activateFeedback(int motorPower, unsigned long duration) {
    buzzTime = millis();
    buzzDuration = duration;
    ledcWrite(MOTOR_PIN, motorPower); 
}

void deactivateFeedback() {
    if(millis() - buzzTime >= buzzDuration) {
        ledcWrite(MOTOR_PIN, 0);
    }
}

//Turn on the external antenna
void turnOnAntenna() {
    pinMode(3, OUTPUT); 
    digitalWrite(3, LOW);  
    pinMode(14, OUTPUT);  
    digitalWrite(14, HIGH); 
    Serial.println("External antenna turned on");
}

void setup() {
    delay(1000);
    Serial.begin(115200);
    Wire.begin();

    // Connect to WiFi
    // wifi.connectToWPAEnterprise(ENTERPRISE_WIFI_SSID, ucsdUsername, ucsdPassword);
    wifi.connectToWiFi(WIFI_SSID, NON_ENTERPRISE_WIFI_PASSWORD);
    macAddress = WiFi.macAddress();
    eventId = EVENT_ID;

    // Connect to MQTT broker and initialize sub/pubs
    if (mqtt.connectToBroker(macAddress, eventId)) {
        mqtt.setCallback([](char* topic, uint8_t* payload, unsigned int length) {
            mqtt.handleMessage(topic, payload, length);
        });
        mqtt.publishAvailability();
        mqtt.subscribeDevice();
        mqtt.subscribeEvent();
        mqtt.subscribeProfileSwap();
        mqtt.publishReceipt("connect", "success");
        Serial.println("Connected to MQTT broker");
    } else {
        Serial.println("Failed to connect to MQTT broker");
    }

    // Connect to NFC tag
    if (!tag.begin(Wire)){
        Serial.println("NFC chip not detected");
        while (1);
    } else {
        Serial.println("NFC chip detected");
        uint8_t tagMemory[256];
        memset(tagMemory, 0, 256);
        tag.writeEEPROM(0x0, tagMemory, 256);
        tag.writeCCFile8Byte();
    }

    // Set up Haptic feedback
    setupFeedback();

    // Turn on external antenna
    turnOnAntenna();
}


void loop() {
    // Maintain MQTT connection
    mqtt.loop();

    // Get assigned ticket ID and load it into the BLE advertising packet
    if(mqtt.isAssigned() && !assigned) {
        ticketId = mqtt.getTicketID();
        mqtt.publishReceipt("assign device", "success");
        assigned = true;
        ble.setTicketId(ticketId);
        ble.setEventId(eventId);
        handshake.init();
        activateFeedback(255, 1000);
    }

    // Write ticket URL to NFC tag 
    if (assigned && !tagWritten) {
        if(tag.writeNDEFURI("youtube.com/watch?v=uSuEdw6HAFE", SFE_ST25DV_NDEF_URI_ID_CODE_HTTPS_WWW)){
            mqtt.publishReceipt("write NFC", "success");
            tagWritten = true;
        }
        else {
            //mqtt.publishReceipt("write NFC", "failure");
            Serial.println("Failed to write NFC tag");
        }
    }

    // If assigned, collect data from the IMU
    if(assigned) {
        handshake.collectData();
    }

    // Process the IMU data and determine if a handshake is detected
    if(assigned && !handshakeDetected) {
        std::vector<std::vector<float>> predictions = handshake.processData();

        if(!predictions.empty() && (int)predictions[0][0] == 4 && predictions[0][1] > 0.9) {
            handshakeDetected = true;
            timeDetected = millis();
            // Serial.println("Handshake detected!");
            mqtt.publishReceipt("handshake", "success");
        }
    }

    // Handshake detected debounce of 2 seconds
    if(handshakeDetected){
        if(millis() - timeDetected >= 2000){
            handshakeDetected = false;
        }
    }

    // If the IMU detects a handshake, advertise/scan over BLE to find the ticket ID of the other device
    if(assigned && !BLEstarted && handshakeDetected) {
        ble.advertise();
        ble.scan();
        BLEstarted = true;
        collectionStart = millis();
    }

    // If BLE is started, collect the strongest signal from the incoming packets
    if(BLEstarted){
        if(millis() - collectionStart >= COLLECTION_TIME){
            ble.stopScanning();
            ble.stopAdvertising();
            BLEstarted = false;
        }

        if (!ble.getIncomingPackets().empty()) {
            std::vector<String> strongest = ble.getIncomingPackets()[0];
        
            for (size_t i = 1; i < ble.getIncomingPackets().size(); i++) {
                int currentRssi = ble.getIncomingPackets()[i][1].toInt(); 
                int strongestRssi = strongest[1].toInt();
            
                if (currentRssi > strongestRssi) {
                    strongest = ble.getIncomingPackets()[i];
                }
            }
            ble.setTicket(strongest[0]);
            // Serial.println("Strongest signal: " + strongest[0] + " at " + strongest[1] + "dBm");
        } 
    }


    //If the BLE scan found a device, upload the associated ticket ID to the MQTT server
    if((ble.getDetectedTicket() != "" ||  prevId != ble.getDetectedTicket()) && !deviceFound) {
        foundId = ble.getDetectedTicket();
        prevId = foundId;
        detectedTickets.push_back(foundId);
        deviceFound = true;

    } else if(deviceFound && !profileExchanged && std::find(detectedTickets.begin(), detectedTickets.end(), foundId) != detectedTickets.end()) {
        mqtt.publishHandshake(foundId);
        mqtt.publishReceipt("profile exchange", "success");
        deviceFound = false;
        profileExchanged = true;
        BLEstarted = false;
    } 

    // Turn off the feedback
    deactivateFeedback();

    // Check for profile swaps and provide haptic feedback if detected
    if(mqtt.profileSwap()){
        activateFeedback(255, 500);
        mqtt.publishReceipt("Haptic feedback", "success");
        mqtt.resetProfileSwapFlag();
    }

    // Check for ticket reassignment
    if(mqtt.ticketReset()){
        assigned = false;
        tagWritten = false;
        profileExchanged = false;
        mqtt.resetTicketFlag();
        mqtt.publishReceipt("ticket_reassignment", "success");
    }

    // Check for NFC reset
    if(mqtt.nfcReset()){
        uint8_t tagMemory[256];
        memset(tagMemory, 0, 256);
        tag.writeEEPROM(0x0, tagMemory, 256);
        tag.writeCCFile8Byte();
        tagWritten = false;
        mqtt.resetNfcFlag();
        mqtt.publishReceipt("nfc_reset", "success");
    }
}

