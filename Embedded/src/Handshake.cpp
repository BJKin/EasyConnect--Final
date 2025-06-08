#include "Handshake.h"
#include "12ClassDense-Multi-Deeper.h"

Handshake::Handshake() : _bno(55, 0x28, &Wire) { 
    _current_index = 0;              
    _samples_collected = 0;         
    _last_sample_time = 0; 
    _tensor_arena = nullptr;
    _model = nullptr;
    _interpreter = nullptr;
    _input_tensor = nullptr;
    _output_tensor = nullptr;
}

Handshake::~Handshake() {
    if (_tensor_arena) {
        free(_tensor_arena);
        _tensor_arena = nullptr;
    }
}

void Handshake::init() {
    if (!_bno.begin()) {
        Serial.println("WARNING: IMU not detected");
        while(1);
    } else {
        Serial.println("IMU detected successfully");
    }

    _tensor_arena = (uint8_t*)malloc(_TENSOR_ARENA_SIZE);
    _model = tflite::GetModel(Handshake_0604_1400_tflite);
    Serial.println("Model loaded successfully");

    static tflite::MicroMutableOpResolver<12> resolver;
    resolver.AddMean();
    resolver.AddAveragePool2D();
    resolver.AddRelu(); 
    resolver.AddSoftmax(); 
    resolver.AddFullyConnected(); 
    resolver.AddShape();
    resolver.AddExpandDims();
    resolver.AddReshape();
    resolver.AddStridedSlice();
    resolver.AddConcatenation();
    resolver.AddMul();
    resolver.AddAdd();
    
    static tflite::MicroInterpreter static_interpreter(
        _model, resolver, _tensor_arena, _TENSOR_ARENA_SIZE);
    _interpreter = &static_interpreter;

    TfLiteStatus allocate_status = _interpreter->AllocateTensors();
    _input_tensor = _interpreter->input(0);
    _output_tensor = _interpreter->output(0);
}


 void Handshake::collectData(){
    imu::Vector<3> accel = _bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    imu::Vector<3> gyro = _bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);

    if (millis() - _last_sample_time >= _SAMPLE_INTERVAL) {
        _last_sample_time = millis(); 

        _data_buffer[_current_index][0] = accel.x();
        _data_buffer[_current_index][1] = accel.y();
        _data_buffer[_current_index][2] = accel.z();
        _data_buffer[_current_index][3] = gyro.x();
        _data_buffer[_current_index][4] = gyro.y();
        _data_buffer[_current_index][5]= gyro.z();
        
        // Serial.print(_data_buffer[_current_index][0], 4);
        // Serial.print(", ");
        // Serial.print(_data_buffer[_current_index][1], 4);
        // Serial.print(", ");
        // Serial.print(_data_buffer[_current_index][2], 4);
        // Serial.print(", ");
        // Serial.print(_data_buffer[_current_index][3], 4);
        // Serial.print(", ");
        // Serial.print(_data_buffer[_current_index][4], 4);
        // Serial.print(", ");
        // Serial.print(_data_buffer[_current_index][5], 4);
        // Serial.print(", ");

        _current_index = (_current_index + 1 ) % _WINDOW_SIZE;

        if (_samples_collected < _WINDOW_SIZE) {
            _samples_collected++;
        } else if (_samples_collected == _WINDOW_SIZE) {
             _inference = true; 
        }
    }
 }

std::vector<std::vector<float>> Handshake::processData() {
    std::vector<std::vector<float>> output;

    if (millis() - _last_process_time >= _PROCESS_INTERVAL && _inference) {
        _last_process_time = millis(); 

        for (int i = 0; i < _WINDOW_SIZE; i++) {
            int buffer_timestep = (_current_index + i) % _WINDOW_SIZE;
            for (int f = 0; f < _NUM_FEATURES; f++) {
                int tensor_position = i * _NUM_FEATURES + f;
                _input_tensor->data.f[tensor_position] = _data_buffer[buffer_timestep][f];
            }
        }
        // unsigned long start_time = millis();
        // Serial.print("Processing data for inference... ");
        TfLiteStatus status = _interpreter->Invoke();
        // unsigned long end_time = millis();
        // Serial.print("Done in ");
        // Serial.print(end_time - start_time);
        // Serial.println(" ms");
        if (status != kTfLiteOk) {
            return output;
        }
        
        float* output_data = _output_tensor->data.f;
        float max_score = output_data[0];
        int predicted_class = 0;
        
        int num_classes = _output_tensor->dims->data[1];
        for (int i = 1; i < num_classes; i++) {
            if (output_data[i] > max_score) {
                max_score = output_data[i];
                predicted_class = i;
            }
        }

    std::vector<float> prediction = {(float)predicted_class, max_score};
    output.push_back(prediction);
    }

    return output;
}

void Handshake::clearBuffer(){
    _current_index = 0;              
    _samples_collected = 0;         
    _last_sample_time = 0; 
    _inference = false;
    
    for (int i = 0; i < _WINDOW_SIZE; i++) {
        for (int j = 0; j < _NUM_FEATURES; j++) {
            _data_buffer[i][j] = 0.0f;
        }
    }
}

// if(!predictions.empty()){
//     String predicted_class_str;
//     switch ((int)predictions[0][0]) {
//         case 0: predicted_class_str = "dancing"; break;
//         case 1: predicted_class_str = "dapup"; break;
//         case 2: predicted_class_str = "fistbump"; break;
//         case 3: predicted_class_str = "flapping"; break;
//         case 4: predicted_class_str = "handshake"; break;
//         case 5: predicted_class_str = "highfive"; break;
//         case 6: predicted_class_str = "scratch"; break;
//         case 7: predicted_class_str = "speedwalking"; break;
//         case 8: predicted_class_str = "still"; break;
//         case 9: predicted_class_str = "stretch"; break;
//         case 10: predicted_class_str = "walking"; break;
//         case 11: predicted_class_str = "waving"; break;
//         default: break;
//     }

//     Serial.print("Predicted: ");
//     Serial.print(predicted_class_str);
//     Serial.print(" (");
//     Serial.print(predictions[0][1] * 100, 1);
//     Serial.println("%)");
// }