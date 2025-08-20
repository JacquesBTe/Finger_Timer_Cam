#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include "../config_pins.h"

class BuzzerControl {
private:
    unsigned long lastBeepTime;
    int beepCount;
    bool isBeeping;
    
public:
    BuzzerControl() {
        lastBeepTime = 0;
        beepCount = 0;
        isBeeping = false;
    }
    
    void init() {
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("Buzzer initialized");
    }
    
    void beep(int duration = 100) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(duration);
        digitalWrite(BUZZER_PIN, LOW);
    }
    
    void beepPattern(int count, int duration = 100, int interval = 150) {
        for (int i = 0; i < count; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(duration);
            digitalWrite(BUZZER_PIN, LOW);
            if (i < count - 1) {
                delay(interval);
            }
        }
    }
    
    void alarmPattern() {
        // Alarm pattern: 3 long beeps
        for (int i = 0; i < 3; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
    }
    
    void update() {
        // For future non-blocking beep implementations
    }
    
    void stop() {
        digitalWrite(BUZZER_PIN, LOW);
        isBeeping = false;
    }
};

#endif