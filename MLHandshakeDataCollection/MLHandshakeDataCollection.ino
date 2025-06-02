#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Wire.h>

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);

const unsigned long SAMPLE_INTERVAL_MS = 10;
unsigned long lastSampleTime = 0;
bool sendIt;
int count;

String receiveMessage() {   
  String message = "";   
  if (Serial.available() > 0) {     
    while (true) { 
    char c = Serial.read();       
    if (c != char(-1)) { 
      if (c == '\n')           
        break;         
        message += c;       
      }    
    }   
  }   
  return message; 
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  if (!bno.begin()) {
    Serial.println("ERROR: Could not find BNO055 sensor!");
    while (1);
  }
  sendIt = false;
  count = 0;
  delay(1000);
}

void loop() {
  String command = receiveMessage();
  if(command == "bigData"){
    sendIt = true;
  }

  if(sendIt){
    if (millis() - lastSampleTime >= SAMPLE_INTERVAL_MS) {
      count += 1;
      lastSampleTime = millis();
      imu::Vector<3> linear_accel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
      imu::Vector<3> gyro = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
      imu::Quaternion quat = bno.getQuat();

      Serial.print(linear_accel.x(), 4);
      Serial.print(",");
      Serial.print(linear_accel.y(), 4);
      Serial.print(",");
      Serial.print(linear_accel.z(), 4);
      Serial.print(",");
      
      Serial.print(gyro.x(), 4);
      Serial.print(",");
      Serial.print(gyro.y(), 4);
      Serial.print(",");
      Serial.print(gyro.z(), 4);
      Serial.print(",");
      
      Serial.print(quat.w(), 4);
      Serial.print(",");
      Serial.print(quat.x(), 4);
      Serial.print(",");
      Serial.print(quat.y(), 4);
      Serial.print(",");
      Serial.println(quat.z(), 4);
    }
  }

  if(count == 300) {
    sendIt = false;
    count = 0;
  }
}

