#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"


#define MQ3_PIN 34
#define SERVO_PIN 13
#define BUZZER_PIN 12
#define LED_PIN 2


const unsigned char alcohol_model[] = { ... };  
tflite::MicroInterpreter interpreter;


const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const String POLICE_API = "https://api.police.example.com/alert";

Servo engineLockServo;
bool engineLocked = false;

void setup() {
    Serial.begin(115200);
    pinMode(MQ3_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);

    
    engineLockServo.attach(SERVO_PIN);
    unlockEngine();

    
    static tflite::AllOpsResolver resolver;
    static uint8_t tensor_arena[2048];
    interpreter = tflite::MicroInterpreter(
        tflite::GetModel(alcohol_model),
        resolver,
        tensor_arena,
        2048
    );
    interpreter.AllocateTensors();

    
    connectToWiFi();
}

void loop() {
    int alcoholLevel = analogRead(MQ3_PIN);
    float temp = readTemperature();  

    
    bool isDrunk = predictAlcohol(alcoholLevel, temp);

    if (isDrunk && !engineLocked) {
        triggerSafetyProtocol(alcoholLevel);
    } else if (!isDrunk && engineLocked) {
        unlockEngine();
    }
    delay(1000);
}


bool predictAlcohol(int sensorVal, float temp) {
    float input[3] = {sensorVal, temp, computeDerivative()};
    interpreter.input(0)->data.f = input;
    interpreter.Invoke();
    return (interpreter.output(0)->data.f[0] > 0.5);
}

void triggerSafetyProtocol(int level) {
    lockEngine();
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 1000, 2000);
    sendPoliceAlert(level);
}

void lockEngine() {
    engineLockServo.write(90);
    engineLocked = true;
}

void unlockEngine() {
    engineLockServo.write(0);
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    engineLocked = false;
}

void sendPoliceAlert(int level) {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    String payload = "{\"level\":" + String(level) + "}";
    http.begin(POLICE_API);
    http.POST(payload);
    http.end();
}
