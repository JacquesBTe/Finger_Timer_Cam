#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "config_pins.h"
#include "ui/oled.h"
#include "ui/buzzer.h"
#include "camera/camera_arducam.h"
#include "logic/state_machine.h"

// Undefine ALL potential macro conflicts from ArduCAM library BEFORE including web server
#ifdef swap
#undef swap
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef round
#undef round
#endif

#include "web/data_collection_server.h"

// Data collection mode - set to false for timer mode
#define DATA_COLLECTION_MODE false
#define USE_PRETRAINED_WEIGHTS true

// Forward declarations
class FingerInference;

// Global objects
OLEDDisplay display;
BuzzerControl buzzer;
ArduCamController camera;
TimerStateMachine stateMachine;
DataCollectionServer webServer;
FingerInference* fingerAI;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=================================");
    Serial.println("   ML-POWERED FINGER TIMER       ");
    Serial.println("=================================");
    
    // Initialize hardware
    Wire.begin(OLED_SDA, OLED_SCL);
    delay(100);
    SPI.begin(CAM_SCK, CAM_MISO, CAM_MOSI, CAM_CS);
    delay(100);
    
    // Initialize components
    bool success = true;
    
    Serial.println("Initializing OLED display...");
    if (!display.init()) {
        Serial.println("✗ OLED initialization failed!");
        success = false;
    } else {
        Serial.println("✓ OLED initialized successfully");
    }
    
    Serial.println("Initializing buzzer...");
    buzzer.init();
    Serial.println("✓ Buzzer initialized");
    
    Serial.println("Initializing camera...");
    if (!camera.init()) {
        Serial.println("✗ Camera initialization failed!");
        success = false;
    } else {
        Serial.println("✓ Camera initialized successfully");
    }
    
    // Initialize ML inference
    Serial.println("Initializing ML inference...");
    // fingerAI = new FingerInference(&camera);
    // fingerAI->printModelInfo();
    Serial.println("✓ ML inference initialized (placeholder)");
    
    // Initialize state machine
    Serial.println("Initializing state machine...");
    stateMachine.init(&display, &buzzer, &camera);
    Serial.println("✓ State machine initialized");
    
    #if DATA_COLLECTION_MODE
    // Initialize web server for data collection
    Serial.println("Initializing web server for data collection...");
    if (webServer.init(&camera)) {
        Serial.println("✓ Web server initialized");
        Serial.println("Data collection web interface available at:");
        Serial.print("http://");
        Serial.println(webServer.getIPAddress());
        String ip = webServer.getIPAddress();
        display.showMessage("Data Collection", ip.c_str());
    } else {
        Serial.println("✗ Web server initialization failed");
        display.showMessage("WiFi Error", "Check credentials");
    }
    #else
    // Normal ML timer operation
    if (success) {
        Serial.println();
        Serial.println("🎉 SETUP COMPLETE - ML TIMER READY!");
        Serial.println("Show 1-5 fingers to set timer minutes");
        display.showMessage("ML Ready!", "Show fingers to set timer");
        buzzer.beep(200);
    } else {
        Serial.println();
        Serial.println("❌ SETUP FAILED!");
        display.showMessage("Error!", "Check connections");
        buzzer.beepPattern(3, 200, 100);
    }
    
    Serial.println();
    Serial.println("ML Timer operation:");
    Serial.println("1. Show fingers (1-5) to camera");
    Serial.println("2. ML model will detect finger count");
    Serial.println("3. Timer starts automatically");
    Serial.println("4. Buzzer sounds when time's up");
    Serial.println();
    #endif
}

// ML inference functions removed - using data collection mode only

// Function declarations
void runMLTimerMode();
void collectTrainingDataWeb();

void collectTrainingDataWeb() {
    if (!webServer.shouldTriggerCollection()) {
        return;
    }
    
    Serial.printf("\n=== WEB DATA COLLECTION ===\n");
    Serial.printf("Collecting sample for %d fingers (Sample %d/%d)\n", 
                 webServer.currentFingers, webServer.currentSample + 1, webServer.samplesPerFingerCount);
    
    // Give visual/audio feedback
    String fingerMsg = String(webServer.currentFingers) + " fingers";
    display.showMessage("Collecting...", fingerMsg.c_str());
    buzzer.beep(100); // Short beep to indicate collection
    
    // Small delay to let user position their hand
    delay(500);
    
    // Extract real camera features (20 features)
    float features[20];
    if (camera.extractImageFeatures(features, 20)) {
        // Create CSV format data
        String csvLine = String(webServer.currentFingers) + ",";
        for (int i = 0; i < 20; i++) {
            csvLine += String(features[i], 2);
            if (i < 19) csvLine += ",";
        }
        
        // Add to web server data collection
        webServer.addDataSample(csvLine);
        
        // Also print to serial for backup
        Serial.println("TRAINING_DATA," + csvLine);
        
        // Print feature preview
        Serial.print("Features: [");
        for (int i = 0; i < 10; i++) { // Show first 10 features
            Serial.printf("%.1f", features[i]);
            if (i < 9) Serial.print(", ");
        }
        Serial.println("...]");
        
        // Update web server state
        webServer.onSampleCollected();
        
        Serial.printf("✓ Sample collected! Total samples: %d\n", webServer.totalSamples);
        
        // Visual feedback
        String sampleMsg = "Sample " + String(webServer.totalSamples);
        display.showMessage(sampleMsg.c_str(), "Collected!");
        buzzer.beepPattern(2, 50, 50); // Success pattern
        
    } else {
        Serial.println("✗ Failed to extract features");
        display.showMessage("Error!", "Feature extraction failed");
        buzzer.beepPattern(3, 100, 100); // Error pattern
    }
}

void loop() {
    if (DATA_COLLECTION_MODE) {
        collectTrainingDataWeb();
    } else {
        // ML Timer Mode - Update state machine with ML inference
        runMLTimerMode();
    }
    
    delay(100);
}

void runMLTimerMode() {
    static unsigned long lastDetection = 0;
    static int consecutiveDetections = 0;
    static int lastDetectedCount = 0;
    
    // Run detection every 1000ms when in waiting state
    TimerState currentState = stateMachine.getState();
    
    if (currentState == STATE_WAITING && (millis() - lastDetection > 1000)) {
        
        Serial.println("Running finger detection...");
        int detectedFingers = 0;
        
        // Use camera's basic finger detection for now
        if (camera.processImageForFingers(detectedFingers)) {
            
            // Enhance the basic detection with some post-processing
            if (detectedFingers > 0 && detectedFingers <= 5) {
                if (detectedFingers == lastDetectedCount) {
                    consecutiveDetections++;
                } else {
                    lastDetectedCount = detectedFingers;
                    consecutiveDetections = 1;
                }
                
                Serial.printf("Detected: %d fingers (consecutive: %d)\n", detectedFingers, consecutiveDetections);
                
                // Show current detection
                display.showFingerCount(detectedFingers);
                
                // If we have 3 consecutive stable detections
                if (consecutiveDetections >= 3) {
                    Serial.printf("Stable detection confirmed: %d fingers\n", detectedFingers);
                    
                    // Set and start timer
                    stateMachine.setTimer(detectedFingers, 0);
                    display.showMessage("Timer Set!", 
                                      String(String(detectedFingers) + " minutes").c_str());
                    buzzer.beepPattern(2, 150, 100);
                    
                    // Brief delay then start timer
                    delay(2000);
                    stateMachine.setState(STATE_RUNNING);
                    
                    // Reset detection state
                    consecutiveDetections = 0;
                    lastDetectedCount = 0;
                }
            } else {
                // Reset if we detect 0 or invalid finger count
                consecutiveDetections = 0;
                lastDetectedCount = 0;
            }
        }
        
        lastDetection = millis();
    }
    
    // Always update the state machine for timer countdown and UI updates
    stateMachine.update();
}