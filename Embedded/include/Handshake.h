#ifndef Handshake_H
#define Handshake_H

#include <Arduino.h>
#include <Adafruit_BNO055.h>
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"

class Handshake {
private:
    Adafruit_BNO055 _bno;

    static const int _WINDOW_SIZE = 125;   
    static const int _NUM_FEATURES = 6;     
    static const int _SAMPLE_INTERVAL = 10;

    float _data_buffer[_WINDOW_SIZE][_NUM_FEATURES];
    int _current_index;              
    int _samples_collected;         
    unsigned long _last_sample_time; 
    unsigned long  _last_process_time;
    static const unsigned long _PROCESS_INTERVAL = 1000; 

    static const int _TENSOR_ARENA_SIZE = 80 * 1024; 
    uint8_t* _tensor_arena;
    const tflite::Model* _model = nullptr;
    tflite::MicroInterpreter* _interpreter = nullptr;
    TfLiteTensor* _input_tensor = nullptr;
    TfLiteTensor* _output_tensor =  nullptr;

    bool _inference = false;
public:
    Handshake();
    ~Handshake();
    void collectData();
    std::vector<std::vector<float>> processData();
    void init();
    void clearBuffer();
};
#endif

