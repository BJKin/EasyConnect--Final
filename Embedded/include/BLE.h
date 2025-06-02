#ifndef BLE_H
#define BLE_H

#include "NimBLEDevice.h"
#include <vector>

class BLE : public NimBLEScanCallbacks {
private:
    std::vector<std::vector<String>> _incomingPackets;
    String _ticketId;
    String _eventId;
    String _detectedTicket;
    String _identifier;
    static const int _RSSI_THRESHOLD = -67;
    unsigned long _collectionStart;
    bool _collecting;
    static const unsigned long _COLLECTION_TIME = 500; 
    void _stopAdvertising();
    void _stopScanning();

public:
    BLE();

    void setTicketId(String ticketID);
    void setEventId(String eventID);
    void advertise();
    void scan();
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override;
    String getDetectedTicket();
    
};

#endif