#include "finger_inference.h"

// Static member definition
float FingerInference::staticFeatures[FingerInference::FEATURE_COUNT];

// Global signal callback for Edge Impulse
static int get_signal_data(size_t offset, size_t length, float* out_ptr) {
    return FingerInference::get_signal_data(offset, length, out_ptr);
}

int FingerInference::runInference() {
    if (!extractImageFeatures()) {
        Serial.println("Failed to extract image features");
        return -1;
    }
    
    // Copy features to static buffer for callback access
    copyFeaturesToStatic();
    
    // Convert features to signal
    signal_t signal;
    signal.total_length = FEATURE_COUNT;
    signal.get_data = ::get_signal_data;
    
    // Run inference
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR inferenceResult = run_classifier(&signal, &result, false);
    
    if (inferenceResult != EI_IMPULSE_OK) {
        Serial.printf("Inference failed: %d\n", inferenceResult);
        return -1;
    }
    
    // Find the class with highest confidence
    int predictedFingers = 0;
    float maxConfidence = 0.0f;
    
    Serial.print("Predictions: ");
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.printf("%s: %.3f ", fingerLabels[i], result.classification[i].value);
        
        if (result.classification[i].value > maxConfidence) {
            maxConfidence = result.classification[i].value;
            predictedFingers = i;
        }
    }
    Serial.println();
    
    // Only accept predictions with sufficient confidence
    if (maxConfidence < EI_CLASSIFIER_THRESHOLD) {
        Serial.printf("Low confidence: %.3f (threshold: %.3f)\n", maxConfidence, EI_CLASSIFIER_THRESHOLD);
        return 0; // Default to no fingers for low confidence
    }
    
    Serial.printf("Predicted: %d fingers (confidence: %.3f)\n", predictedFingers, maxConfidence);
    
    // Add to smoothing buffer
    addToSmoothingBuffer(predictedFingers);
    
    // Return smoothed prediction
    return getSmoothedPrediction();
}