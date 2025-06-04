#include "BLE.h"

BLE::BLE() {
    _identifier = BLE_IDENTIFIER;
    _collecting = false;
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
        //Serial.println("Advertising started successfully");
    } else {
        //Serial.println("Failed to start advertising");
    }
}

void BLE::scan() {
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan -> setScanCallbacks(this);

    if(scan->start(0, false)){
        //Serial.println("Scanning started successfully");
    } else {
        //Serial.println("Failed to start scanning");
    }
}

void BLE::onResult(const NimBLEAdvertisedDevice* device) {
    if (String(device->getName().c_str()) == "EZ") {
        if (!_collecting) {
            _collecting = true;
            _collectionStart = millis();
            //Serial.println("Starting signal collection...");
        }
        std::vector<uint8_t> payload = device->getPayload();
        std::string payloadStr(payload.begin(), payload.end()); 
        String payloadArdStr = String(payloadStr.substr(2, 21).c_str());

        //Serial.println("Detected RSSI: " + String(device->getRSSI()) + " Threshold: " + String(_RSSI_THRESHOLD));
        //Serial.println(device->getRSSI() > _RSSI_THRESHOLD);

        if(payloadArdStr.substring(2,8) == _identifier && payloadArdStr.substring(8,12) == _eventId && device->getRSSI() > _RSSI_THRESHOLD) {
            //Serial.println("Valid advertisement detected from: " + String(device->getName().c_str()));
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
                //Serial.println("Detected ticket: " + detectedTicket + " RSSI: " + rssi);
            } 

            if (_collecting && (millis() - _collectionStart >= _COLLECTION_TIME)) {
            _collecting = false;
        
                if (!_incomingPackets.empty()) {
                    std::vector<String> strongest = _incomingPackets[0];
                
                    for (size_t i = 1; i < _incomingPackets.size(); i++) {
                        int currentRssi = _incomingPackets[i][1].toInt(); 
                        int strongestRssi = strongest[1].toInt();
                    
                        if (currentRssi > strongestRssi) {
                            strongest = _incomingPackets[i];
                        }
                    }
                    _detectedTicket = strongest[0];
                    //Serial.println("Strongest signal: " + strongest[0] + " at " + strongest[1] + "dBm");
                    _stopScanning();
                    _stopAdvertising();
                } else {
                //Serial.println("No devices detected");
                _stopScanning();
                _stopAdvertising();
                }
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

void BLE::_stopAdvertising() {
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->stop();
    // Serial.println("Advertising stopped");
}

void BLE::_stopScanning() {
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->stop();
    _incomingPackets.clear();
    // Serial.println("Scanning stopped");
}

