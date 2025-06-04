#include "ECE140_MQTT.h"
#include "Certificates.h"


ECE140_MQTT::ECE140_MQTT(){
    _username = MQTT_USERNAME;
    _password = MQTT_PASSWORD;

    _wifiClient.setCACert(CA_CERT); 
    _mqttClient = new PubSubClient(_wifiClient);
    _mqttClient->setServer(MQTT_SERVER, atoi(MQTT_PORT));
    _mqttClient->setBufferSize(1024);
    Serial.println("[ECE140_MQTT] Initialized");
}

bool ECE140_MQTT::connectToBroker(String clientID, String eventID) {
    _clientId = clientID;
    _eventId = eventID;
    
    Serial.println("[MQTT] Connecting to HiveMQ broker...");
    
    if (_mqttClient->connect(_clientId.c_str(), _username.c_str(), _password.c_str())) {
        Serial.println("[MQTT] Connected successfully!");
        return true;
    } else {
        Serial.print("[MQTT] Connection failed with state: ");
        Serial.println(_mqttClient->state());
        return false;
    }
}

bool ECE140_MQTT::publishAvailability() {
    String fullTopic = "event/" + _eventId + "/available_devices/" + _clientId;

    String payload = "{\"event_id\": " + _eventId +
                    ", \"device_id\": " + _clientId + 

                    ", \"is_available\": true}";
    
    if (_mqttClient->publish(fullTopic.c_str(), payload.c_str())) {
        Serial.println("[MQTT] Availability published successfully");
        return true;
    } else {
        Serial.println("[MQTT] Failed to publish availability");
        return false;
    }
}

bool ECE140_MQTT::publishReceipt(String command, String status) {
    String fullTopic = "device/" + _clientId + "/receipt";

    String payload = "{\"command\": " + command +
                    ", \"status\": " + status + "}";
    
    if (_mqttClient->publish(fullTopic.c_str(), payload.c_str())) {
        Serial.println("[MQTT] Receipt published successfully");
        return true;
    } else {
        Serial.println("[MQTT] Failed to publish receipt");
        return false;
    }
}

bool ECE140_MQTT::publishHandshake(String ticketID) {
    String fullTopic = "event/" + _eventId + "/profile_swap";

    String payload = "{\"event_id\": " + _eventId +
                    ", \"ticket_id\": " + _ticketId +
                    ", \"ticket_id_to_swap\": " + ticketID + "}";
    
    if (_mqttClient->publish(fullTopic.c_str(), payload.c_str())) {
        Serial.println("[MQTT] Profile swap published successfully");
        return true;
    } else {
        Serial.println("[MQTT] Failed to publish profile swap");
        return false;
    }
}

bool ECE140_MQTT::subscribeDevice() {
    String fullTopic = "device/" + _clientId + "/#";
    
    if (_mqttClient->subscribe(fullTopic.c_str())) {
        Serial.println("[MQTT] Subscribed successfully to assignment");
        return true;
    } else {
        Serial.println("[MQTT] Failed to subscribe to assignment");
        return false;
    }
}

bool ECE140_MQTT::subscribeEvent() {
    String fullTopic = "event/" + _eventId + "/#";
    
    if (_mqttClient->subscribe(fullTopic.c_str())) {
        Serial.println("[MQTT] Subscribed successfully to event notifications");
        return true;
    } else {
        Serial.println("[MQTT] Failed to subscribe to event notifications");
        return false;
    }
}

bool ECE140_MQTT::subscribeProfileSwap() {
    String fullTopic = "event/" + _eventId + "/handshake_success/#";  
    
    if (_mqttClient->subscribe(fullTopic.c_str())) {
        Serial.println("[MQTT] Subscribed successfully to profile swap");
        return true;
    } else {
        Serial.println("[MQTT] Failed to subscribe to profile swap");
        return false;
    }
}

void ECE140_MQTT::handleMessage(char* topic, uint8_t* payload, unsigned int length) {
    String topicStr = String(topic);
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    // Serial.println("=== MQTT DEBUG ===");
    // Serial.println("Topic: " + topicStr);
    // Serial.println("Payload: " + message);
    // Serial.println("==================");
    
    if (topicStr == "device/" + _clientId + "/assignment") {
        _ticketId = message;
    } else if(topicStr == "device/" + _clientId + "/reboot"){
        publishReceipt("reboot", "acknowledged");
        delay(100);
        ESP.restart();
    } else if(topicStr == "device/" + _clientId + "/reassignment"){
         _ticketId = message;
         _ticketReset = true;
    } else if(topicStr == "device/" + _clientId + "/resetNFC"){
         _nfcReset = true;
    } else if (topicStr == "event/" + _eventId + "/reboot") {
        publishReceipt("reboot", "acknowledged");
        delay(100);
        ESP.restart();
    } else if (topicStr.substring(0, 22) == "event/" + _eventId + "/profile_swap") {
        if(topicStr.substring(23) == _ticketId){
            _profileSwap = true;
        }
    } 
}

void ECE140_MQTT::setCallback(void (*callback)(char*, uint8_t*, unsigned int)) {
    _mqttClient->setCallback(callback);
}

void ECE140_MQTT::loop() {
    _mqttClient->loop();
    
    // Reconnect if connection is lost
    if (!_mqttClient->connected()) {
        Serial.println("[MQTT] Connection lost. Attempting to reconnect...");
        connectToBroker(_clientId, _eventId);
    }
}

bool ECE140_MQTT::isAssigned() {
    return !_ticketId.isEmpty();
}

String ECE140_MQTT::getTicketID() {
    return _ticketId;
}

bool ECE140_MQTT::ticketReset() {
    return _ticketReset;
}

bool ECE140_MQTT::nfcReset() {
    return _nfcReset;
}

void ECE140_MQTT::resetNfcFlag(){
    _nfcReset = !_nfcReset;
}

void ECE140_MQTT::resetTicketFlag(){
    _ticketReset = !_ticketReset;
}

bool ECE140_MQTT::profileSwap(){
   return _profileSwap;
}

void ECE140_MQTT::resetProfileSwapFlag(){
   _profileSwap = !_profileSwap;
}