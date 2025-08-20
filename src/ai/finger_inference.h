#ifndef FINGER_INFERENCE_H
#define FINGER_INFERENCE_H

#include <Arduino.h>
#include "../ESP32-Finger_Counter_inferencing.h"
#include "../camera/camera_arducam.h"

class FingerInference {
private:
    static constexpr int FEATURE_COUNT = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    static constexpr int SMOOTHING_WINDOW = 5;
    
    float features[FEATURE_COUNT];
    ArduCamController* camera;
    
    // Smoothing for stable predictions
    int recentPredictions[SMOOTHING_WINDOW];
    int predictionIndex;
    bool smoothingBufferFull;
    
    // Labels for classification results
    const char* fingerLabels[EI_CLASSIFIER_LABEL_COUNT] = {
        "no_fingers", "one_finger", "two_fingers", 
        "three_fingers", "four_fingers", "five_fingers"
    };

public:
    FingerInference(ArduCamController* cam) {
        camera = cam;
        predictionIndex = 0;
        smoothingBufferFull = false;
        
        // Initialize smoothing buffer
        for (int i = 0; i < SMOOTHING_WINDOW; i++) {
            recentPredictions[i] = 0;
        }
        
        Serial.println("Finger Inference initialized");
        Serial.printf("Feature count: %d\n", FEATURE_COUNT);
        Serial.printf("Label count: %d\n", EI_CLASSIFIER_LABEL_COUNT);
    }
    
    bool extractImageFeatures() {
        if (!camera) {
            Serial.println("Camera not available");
            return false;
        }
        
        // Use camera's feature extraction method
        return camera->extractImageFeatures(features, FEATURE_COUNT);
    }
    
    int runInference();
    
    int getFingerCount() {
        return runInference();
    }
    
    void printModelInfo() {
        Serial.println("=== Edge Impulse Model Info ===");
        Serial.printf("Project: %s\n", EI_CLASSIFIER_PROJECT_NAME);
        Serial.printf("Version: %d\n", EI_CLASSIFIER_PROJECT_DEPLOY_VERSION);
        Serial.printf("Input frame size: %d\n", EI_CLASSIFIER_NN_INPUT_FRAME_SIZE);
        Serial.printf("Output count: %d\n", EI_CLASSIFIER_NN_OUTPUT_COUNT);
        Serial.printf("Threshold: %.3f\n", EI_CLASSIFIER_THRESHOLD);
        Serial.printf("Interval: %d ms\n", EI_CLASSIFIER_INTERVAL_MS);
        Serial.printf("Arena size: %d bytes\n", EI_CLASSIFIER_TFLITE_LARGEST_ARENA_SIZE);
        Serial.println("Labels:");
        for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            Serial.printf("  %d: %s\n", i, fingerLabels[i]);
        }
        Serial.println("===============================");
    }

private:
    void addToSmoothingBuffer(int prediction) {
        recentPredictions[predictionIndex] = prediction;
        predictionIndex = (predictionIndex + 1) % SMOOTHING_WINDOW;
        
        if (predictionIndex == 0) {
            smoothingBufferFull = true;
        }
    }
    
    int getSmoothedPrediction() {
        if (!smoothingBufferFull && predictionIndex < 3) {
            // Not enough data for smoothing, return latest
            return recentPredictions[(predictionIndex - 1 + SMOOTHING_WINDOW) % SMOOTHING_WINDOW];
        }
        
        // Count occurrences of each prediction
        int counts[EI_CLASSIFIER_LABEL_COUNT] = {0};
        int samplesUsed = smoothingBufferFull ? SMOOTHING_WINDOW : predictionIndex;
        
        for (int i = 0; i < samplesUsed; i++) {
            if (recentPredictions[i] < EI_CLASSIFIER_LABEL_COUNT) {
                counts[recentPredictions[i]]++;
            }
        }
        
        // Find most frequent prediction
        int mostFrequent = 0;
        int maxCount = counts[0];
        for (int i = 1; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            if (counts[i] > maxCount) {
                maxCount = counts[i];
                mostFrequent = i;
            }
        }
        
        Serial.printf("Smoothed prediction: %d fingers (from %d samples)\n", mostFrequent, samplesUsed);
        return mostFrequent;
    }
    
    // Static features buffer for signal callback
    static float staticFeatures[FEATURE_COUNT];
    
    void copyFeaturesToStatic() {
        memcpy(staticFeatures, features, sizeof(features));
    }

public:
    static int get_signal_data(size_t offset, size_t length, float* out_ptr) {
        for (size_t i = 0; i < length; i++) {
            if (offset + i < FEATURE_COUNT) {
                out_ptr[i] = staticFeatures[offset + i];
            } else {
                out_ptr[i] = 0.0f;
            }
        }
        return 0;
    }
};

// Static member definition will be in the cpp file

#endif