#include "BLE.h"

BLE::BLE() {
    _identifier = BLE_IDENTIFIER;
}

void BLE::advertise(){
    NimBLEDevice::init("");
    NimBLEAdvertisementData advData;
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();

    String stringData = _identifier + _eventId + _ticketId;
    advData.setName("EZ");
    advData.addData((uint8_t *)stringData.c_str(), stringData.length()); 
    adv->setAdvertisementData(advData);

    bool started = adv->start();
    if (started) {
        Serial.println("Advertising started successfully");
    } else {
        //Serial.println("Failed to start advertising");
    }
}

void BLE::scan() {
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan -> setScanCallbacks(this);

    if(scan->start(0, false)){
        Serial.println("Scanning started successfully");
    } else {
        //Serial.println("Failed to start scanning");
    }
}

void BLE::onResult(const NimBLEAdvertisedDevice* device) {
    if (String(device->getName().c_str()) == "EZ") {
        std::vector<uint8_t> payload = device->getPayload();
        std::string payloadStr(payload.begin(), payload.end()); 
        String payloadArdStr = String(payloadStr.substr(2, 21).c_str());

        Serial.println("Detected RSSI: " + String(device->getRSSI()) + " Threshold: " + String(_RSSI_THRESHOLD));
        Serial.println(device->getRSSI() > _RSSI_THRESHOLD);

        if(payloadArdStr.substring(2,8) == _identifier && payloadArdStr.substring(8,12) == _eventId && device->getRSSI() > _RSSI_THRESHOLD) {
            Serial.println("Valid advertisement detected from: " + String(device->getName().c_str()));
            String detectedTicket = payloadArdStr.substring(12,21);
            String rssi = String(device->getRSSI());
            bool found = false;
            for (const auto& packet : _incomingPackets) {
                if (packet[0] == detectedTicket) {  
                    found = true;
                    break;
                }
            }

            if (!found) {  
                std::vector<String> newPacket = {detectedTicket, rssi};
                _incomingPackets.push_back(newPacket); 
                Serial.println("Detected ticket: " + detectedTicket + " RSSI: " + rssi);
            } 
        }
    }
}

String BLE::getDetectedTicket() {
    return _detectedTicket;
}

void BLE::setTicketId(String ticketID){
    _ticketId = ticketID;
}

void BLE::setEventId(String eventID){
    _eventId = eventID;
}

void BLE::stopAdvertising() {
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->stop();
    Serial.println("Advertising stopped");
}

void BLE::stopScanning() {
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->stop();
    _incomingPackets.clear();
    Serial.println("Scanning stopped");
}

std::vector<std::vector<String>> BLE::getIncomingPackets() {
    return _incomingPackets;
}

void BLE::setTicket(String ticket) {
    _detectedTicket = ticket;
}

