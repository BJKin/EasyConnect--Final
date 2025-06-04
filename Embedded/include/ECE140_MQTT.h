#ifndef ECE140_MQTT_H
#define ECE140_MQTT_H

#include <WiFi.h>
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

class ECE140_MQTT {
private:
    String _clientId;
    String _username;
    String _password;
    String _eventId;
    String _ticketId;
    WiFiClientSecure _wifiClient;
    PubSubClient* _mqttClient;
    bool _ticketReset = false;
    bool _nfcReset = false;
    bool _profileSwap = false;

public:
    ECE140_MQTT();
    
    bool connectToBroker(String clientID, String eventID);
    bool subscribeDevice();
    bool subscribeEvent();
    bool publishReceipt(String command, String status);
    bool publishAvailability();
    bool publishHandshake(String deviceID);
    void setCallback(void (*callback)(char*, uint8_t*, unsigned int));
    void loop();
    void handleMessage(char* topic,  uint8_t* payload, unsigned int length);
    bool isAssigned();
    bool ticketReset();
    bool nfcReset();
    void resetNfcFlag();
    void resetTicketFlag();
    bool subscribeProfileSwap();
    bool profileSwap();
    void resetProfileSwapFlag();
    String getTicketID();
};

#endif