#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>

enum TimerState {
    STATE_WAITING,      // Waiting for finger input
    STATE_DETECTING,    // Detecting fingers
    STATE_SETTING,      // Setting timer based on fingers
    STATE_RUNNING,      // Timer counting down
    STATE_FINISHED      // Timer completed
};

// Forward declarations
class OLEDDisplay;
class BuzzerControl;
class ArduCamController;

class TimerStateMachine {
private:
    TimerState currentState;
    unsigned long stateStartTime;
    unsigned long timerStartTime;
    unsigned long timerDuration;    // in milliseconds
    int detectedFingers;
    int confirmationCount;
    
    // References to hardware components
    OLEDDisplay* display;
    BuzzerControl* buzzer;
    ArduCamController* camera;
    
    // State timeouts
    static const unsigned long DETECTION_TIMEOUT = 3000;   // 3 seconds
    static const unsigned long SETTING_TIMEOUT = 2000;     // 2 seconds
    static const unsigned long CONFIRMATION_REQUIRED = 3;   // Need 3 consistent readings
    
public:
    TimerStateMachine() {
        currentState = STATE_WAITING;
        stateStartTime = 0;
        timerStartTime = 0;
        timerDuration = 0;
        detectedFingers = 0;
        confirmationCount = 0;
        display = nullptr;
        buzzer = nullptr;
        camera = nullptr;
    }
    
    void init(OLEDDisplay* disp, BuzzerControl* buzz, ArduCamController* cam) {
        display = disp;
        buzzer = buzz;
        camera = cam;
        setState(STATE_WAITING);
    }
    
    void update() {
        switch (currentState) {
            case STATE_WAITING:
                handleWaitingState();
                break;
            case STATE_DETECTING:
                handleDetectingState();
                break;
            case STATE_SETTING:
                handleSettingState();
                break;
            case STATE_RUNNING:
                handleRunningState();
                break;
            case STATE_FINISHED:
                handleFinishedState();
                break;
        }
    }
    
    // State management
    void setState(TimerState newState) {
        currentState = newState;
        stateStartTime = millis();
        Serial.printf("State changed to: %d\n", newState);
    }
    
    TimerState getState() const { return currentState; }
    
    // Timer functions
    void setTimer(int minutes, int seconds = 0) {
        timerDuration = (minutes * 60 + seconds) * 1000; // Convert to milliseconds
        Serial.printf("Timer set for %d minutes %d seconds\n", minutes, seconds);
    }
    
    unsigned long getRemainingTime() {
        if (currentState != STATE_RUNNING) return 0;
        unsigned long elapsed = millis() - timerStartTime;
        if (elapsed >= timerDuration) return 0;
        return (timerDuration - elapsed) / 1000; // Return seconds
    }
    
    bool isTimerExpired() {
        if (currentState != STATE_RUNNING) return false;
        return (millis() - timerStartTime) >= timerDuration;
    }
    
    // Finger detection
    void processFingerCount(int count) {
        if (count == detectedFingers) {
            confirmationCount++;
        } else {
            detectedFingers = count;
            confirmationCount = 1;
        }
    }
    
    int getDetectedFingers() const { return detectedFingers; }
    
private:
    void handleWaitingState() {
        // ML detection is now handled in main loop
        // This state just waits for external trigger
        static unsigned long lastMessage = 0;
        if (millis() - lastMessage > 5000) {
            if (display) {
                display->showMessage("Ready!", "Show fingers to set timer");
            }
            lastMessage = millis();
        }
    }
    
    void handleDetectingState() {
        // ML detection is now handled in main loop
        // This state is mostly bypassed now
        setState(STATE_WAITING);
    }
    
    void handleSettingState() {
        // Timer is being set, show countdown before starting
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 500) {
            int remaining = SETTING_TIMEOUT - (millis() - stateStartTime);
            remaining /= 1000;
            
            if (display) {
                display->showTimer(detectedFingers, 0);
            }
            
            lastUpdate = millis();
        }
        
        if (millis() - stateStartTime > SETTING_TIMEOUT) {
            setTimer(detectedFingers, 0);
            setState(STATE_RUNNING);
            startTimer();
        }
    }
    
    void handleRunningState() {
        unsigned long remaining = getRemainingTime();
        
        if (display) {
            display->showCountdown(timerDuration / 1000, remaining);
        }
        
        if (isTimerExpired()) {
            setState(STATE_FINISHED);
            finishTimer();
        }
    }
    
    void handleFinishedState() {
        static unsigned long lastBeep = 0;
        
        if (display) {
            display->showFinished();
        }
        
        // Beep every 2 seconds
        if (millis() - lastBeep > 2000) {
            if (buzzer) {
                buzzer->alarmPattern();
            }
            lastBeep = millis();
        }
        
        // Reset after 10 seconds
        if (millis() - stateStartTime > 10000) {
            setState(STATE_WAITING);
            if (display) {
                display->showMessage("Ready!", "Show fingers to set timer");
            }
        }
    }
    
    void startDetection() {
        Serial.println("Starting finger detection");
        confirmationCount = 0;
    }
    
    void confirmFingerCount(int count) {
        Serial.printf("Finger count confirmed: %d\n", count);
        if (buzzer) {
            buzzer->beep(200);
        }
    }
    
    void startTimer() {
        timerStartTime = millis();
        Serial.println("Timer started");
        if (buzzer) {
            buzzer->beepPattern(2, 100, 100);
        }
    }
    
    void finishTimer() {
        Serial.println("Timer finished!");
        if (buzzer) {
            buzzer->alarmPattern();
        }
    }
};

#endif