#ifndef CAMERA_ARDUCAM_H
#define CAMERA_ARDUCAM_H

#include <ArduCAM.h>
#include <Wire.h>
#include <SPI.h>
#include "../config_pins.h"
#include "downscale.h"

// Define MAX_FIFO_SIZE if not already defined by ArduCAM library
#ifndef MAX_FIFO_SIZE
#define MAX_FIFO_SIZE 0x5FFFF  // 384KB max FIFO size
#endif

class ArduCamController {
private:
    ArduCAM* myCAM;
    bool isInitialized;
    uint8_t* imageBuffer;
    size_t bufferSize;
    
    bool testSPI() {
        // Test SPI by writing and reading back a test pattern
        // Try multiple register addresses for different ArduCAM variants
        
        Serial.println("Testing SPI with ARDUCHIP_TEST1 register...");
        myCAM->write_reg(ARDUCHIP_TEST1, 0x55);
        delay(1);
        uint8_t temp = myCAM->read_reg(ARDUCHIP_TEST1);
        Serial.printf("  Wrote: 0x55, Read: 0x%02X\n", temp);
        
        if (temp == 0x55) {
            myCAM->write_reg(ARDUCHIP_TEST1, 0xAA);
            delay(1);
            temp = myCAM->read_reg(ARDUCHIP_TEST1);
            Serial.printf("  Wrote: 0xAA, Read: 0x%02X\n", temp);
            
            if (temp == 0xAA) {
                Serial.println("✓ SPI test passed with ARDUCHIP_TEST1");
                return true;
            }
        }
        
        // Try alternative test register for PSRAM boards
        Serial.println("Testing SPI with alternative register (0x40)...");
        myCAM->write_reg(0x40, 0x55);
        delay(1);
        temp = myCAM->read_reg(0x40);
        Serial.printf("  Wrote: 0x55, Read: 0x%02X\n", temp);
        
        if (temp == 0x55) {
            myCAM->write_reg(0x40, 0xAA);
            delay(1);
            temp = myCAM->read_reg(0x40);
            Serial.printf("  Wrote: 0xAA, Read: 0x%02X\n", temp);
            
            if (temp == 0xAA) {
                Serial.println("✓ SPI test passed with register 0x40");
                return true;
            }
        }
        
        // Try with different SPI settings for PSRAM boards
        Serial.println("Testing SPI with slower clock for PSRAM compatibility...");
        SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // Slower clock
        
        digitalWrite(CAM_CS, LOW);
        delayMicroseconds(10);
        SPI.transfer(ARDUCHIP_TEST1 | 0x80); // Write
        SPI.transfer(0x55);
        digitalWrite(CAM_CS, HIGH);
        delayMicroseconds(10);
        
        digitalWrite(CAM_CS, LOW);
        delayMicroseconds(10);
        SPI.transfer(ARDUCHIP_TEST1 & 0x7F); // Read
        temp = SPI.transfer(0x00);
        digitalWrite(CAM_CS, HIGH);
        
        SPI.endTransaction();
        
        Serial.printf("  Slow SPI - Wrote: 0x55, Read: 0x%02X\n", temp);
        
        if (temp == 0x55) {
            Serial.println("✓ SPI test passed with slow clock");
            return true;
        }
        
        Serial.println("✗ All SPI tests failed");
        Serial.println("Your board has PSRAM (Winband W9864KH-6) which may need special handling");
        return false;
    }
    
    bool testI2C() {
        // Test I2C by reading camera ID
        uint8_t vid, pid;
        myCAM->wrSensorReg8_8(0xff, 0x01);  // Select register bank
        myCAM->rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
        myCAM->rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
        
        Serial.printf("Camera ID detected: 0x%02X 0x%02X\n", vid, pid);
        
        // Check for OV2640 variants
        if (vid != 0x26) {
            Serial.printf("I2C test failed. Expected VID: 0x26, Got: 0x%02X\n", vid);
            return false;
        }
        
        // Accept common OV2640 PID variants
        if (pid == 0x42) {
            Serial.println("I2C test passed - OV2640 detected (standard)");
            return true;
        } else if (pid == 0x41) {
            Serial.println("I2C test passed - OV2640 detected (variant)");
            return true;
        } else {
            Serial.printf("I2C test failed. Unknown PID: 0x%02X (expected 0x41 or 0x42)\n", pid);
            return false;
        }
    }
    
public:
    ArduCamController() {
        myCAM = nullptr;
        isInitialized = false;
        imageBuffer = nullptr;
        bufferSize = CAM_WIDTH * CAM_HEIGHT; // Grayscale
    }
    
    ~ArduCamController() {
        if (imageBuffer) {
            free(imageBuffer);
        }
        if (myCAM) {
            delete myCAM;
        }
    }
    
    bool init() {
        Serial.println("Initializing ArduCAM with PSRAM support...");
        
        // Allocate image buffer
        imageBuffer = (uint8_t*)malloc(bufferSize);
        if (!imageBuffer) {
            Serial.println("Failed to allocate image buffer");
            return false;
        }
        
        // Initialize camera with OV2640 and correct pins
        myCAM = new ArduCAM(OV2640, CAM_CS);
        
        // Add longer delay for PSRAM boards
        delay(500);
        
        // Test SPI connection with PSRAM-aware tests
        if (!testSPI()) {
            Serial.println("SPI test failed");
            return false;
        }
        
        // Test I2C connection
        if (!testI2C()) {
            Serial.println("I2C test failed");
            return false;
        }
        
        // PSRAM boards may need special reset sequence
        Serial.println("Performing PSRAM-compatible reset...");
        myCAM->write_reg(0x07, 0x80);  // Reset
        delay(200);  // Longer delay for PSRAM
        myCAM->write_reg(0x07, 0x00);  // Normal mode
        delay(200);
        
        // Set to JPEG mode
        myCAM->set_format(JPEG);
        myCAM->InitCAM();
        
        // For PSRAM boards, try lower resolution first
        myCAM->OV2640_set_JPEG_size(OV2640_160x120);
        
        // Additional delay for PSRAM initialization
        delay(1000);
        
        // Test FIFO operations for PSRAM boards
        Serial.println("Testing FIFO operations...");
        myCAM->flush_fifo();
        myCAM->clear_fifo_flag();
        
        isInitialized = true;
        Serial.println("ArduCAM with PSRAM initialized successfully");
        return true;
    }
    
    bool captureImage() {
        if (!isInitialized) {
            Serial.println("Camera not initialized");
            return false;
        }
        
        // Start capture
        myCAM->flush_fifo();
        myCAM->clear_fifo_flag();
        myCAM->start_capture();
        
        // Wait for capture to complete
        while (!myCAM->get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
            delay(1);
        }
        
        Serial.println("Image captured");
        return true;
    }
    
    bool processImageForFingers(int& fingerCount) {
        if (!captureImage()) {
            return false;
        }
        
        // Read FIFO length - handle PSRAM board quirks
        uint32_t length = myCAM->read_fifo_length();
        Serial.printf("Raw FIFO length: %d\n", length);
        
        // PSRAM boards sometimes report incorrect FIFO length
        if (length >= MAX_FIFO_SIZE || length == 0 || length > 100000) {
            Serial.printf("Invalid FIFO length detected: %d\n", length);
            Serial.println("Using fixed length for PSRAM board...");
            length = 8192; // Use a reasonable fixed size for 160x120 JPEG
        }
        
        Serial.printf("Using FIFO length: %d bytes\n", length);
        
        // Enhanced finger detection algorithm
        myCAM->CS_LOW();
        myCAM->set_fifo_burst();
        
        // Skip JPEG header (first ~200 bytes)
        for (int i = 0; i < 200 && i < length; i++) {
            SPI.transfer(0x00);
        }
        
        // Sample image data in a grid pattern to detect finger-like regions
        uint32_t sampleCount = (3000 < length) ? 3000 : length;
        uint8_t samples[100]; // Store sample points
        int sampleIndex = 0;
        
        // Take samples across the image
        for (int i = 0; i < sampleCount && sampleIndex < 100; i += 30) {
            uint8_t pixel = SPI.transfer(0x00);
            if (sampleIndex < 100) {
                samples[sampleIndex++] = pixel;
            }
        }
        
        myCAM->CS_HIGH();
        
        // Analyze samples for finger detection
        fingerCount = analyzeImageSamples(samples, sampleIndex);
        
        Serial.printf("Analyzed %d samples, detected finger count: %d\n", sampleIndex, fingerCount);
        return true;
    }
     bool extractImageFeatures(float* features, int feature_count) {
        if (!captureImage()) {
            return false;
        }
        
        uint32_t length = myCAM->read_fifo_length();
        if (length > 100000) length = 8192; // Handle PSRAM quirk
        
        myCAM->CS_LOW();
        myCAM->set_fifo_burst();
        
        // Skip JPEG header
        for (int i = 0; i < 200 && i < length; i++) {
            SPI.transfer(0x00);
        }
        
        // Create a simple pixel buffer to store image data
        uint8_t pixels[400]; // Store 400 pixels for analysis
        int pixel_count = 0;
        
        // Read actual pixel data from the image
        int dataSize = length - 200;
        for (int i = 0; i < dataSize && pixel_count < 400; i++) {
            uint8_t pixel = SPI.transfer(0x00);
            if (i % (dataSize / 400 + 1) == 0) { // Sample every nth pixel
                pixels[pixel_count++] = pixel;
            }
        }
        
        myCAM->CS_HIGH();
        
        if (pixel_count < 100) {
            // Not enough data, return default features
            for (int i = 0; i < feature_count; i++) {
                features[i] = 128.0; // Neutral value
            }
            return false;
        }
        
        // Extract real statistical features from the image
        for (int i = 0; i < feature_count && i < 20; i++) {
            // Analyze different regions of the pixel array
            int region_size = pixel_count / feature_count;
            int region_start = i * region_size;
            
            uint32_t sum = 0;
            uint32_t min_val = 255;
            uint32_t max_val = 0;
            int edge_count = 0;
            
            for (int j = 0; j < region_size && (region_start + j) < pixel_count; j++) {
                uint8_t pixel = pixels[region_start + j];
                sum += pixel;
                
                if (pixel < min_val) min_val = pixel;
                if (pixel > max_val) max_val = pixel;
                
                // Count edges (significant brightness changes)
                if (j > 0) {
                    uint8_t prev_pixel = pixels[region_start + j - 1];
                    if (abs((int)pixel - (int)prev_pixel) > 30) {
                        edge_count++;
                    }
                }
            }
            
            // Calculate different feature types
            if (region_size > 0) {
                uint8_t avg = sum / region_size;
                uint8_t contrast = max_val - min_val;
                float edge_density = (float)edge_count / region_size;
                
                // Create diverse features for better ML training
                if (i < 7) {
                    features[i] = (float)avg; // Average brightness
                } else if (i < 14) {
                    features[i] = (float)contrast; // Local contrast
                } else {
                    features[i] = edge_density * 100; // Edge density (0-100 range)
                }
            } else {
                features[i] = 128.0;
            }
        }
        
        // Normalize features to 0-255 range for consistent training
        for (int i = 0; i < feature_count; i++) {
            if (features[i] < 0) features[i] = 0;
            if (features[i] > 255) features[i] = 255;
        }
        
        return true;
    }
    
    int analyzeImageSamples(uint8_t* samples, int count) {
        if (count < 10) return 0; // Not enough data
        
        // Calculate image statistics
        int sum = 0;
        int darkPixels = 0;
        int brightPixels = 0;
        int transitions = 0;
        
        for (int i = 0; i < count; i++) {
            sum += samples[i];
            
            // Count dark and bright regions
            if (samples[i] < 80) darkPixels++;
            else if (samples[i] > 180) brightPixels++;
            
            // Count transitions (edges)
            if (i > 0 && abs(samples[i] - samples[i-1]) > 40) {
                transitions++;
            }
        }
        
        int avgBrightness = sum / count;
        float darkRatio = (float)darkPixels / count;
        float brightRatio = (float)brightPixels / count;
        float transitionRatio = (float)transitions / count;
        
        Serial.printf("Image analysis - Avg brightness: %d, Dark ratio: %.2f, Bright ratio: %.2f, Transitions: %.2f\n", 
                     avgBrightness, darkRatio, brightRatio, transitionRatio);
        
        // Improved finger detection logic
        
        // No hand detected - very uniform image
        if (transitionRatio < 0.1) {
            return 0;
        }
        
        // Very bright or very dark - probably not a hand
        if (avgBrightness < 30 || avgBrightness > 220) {
            return 0;
        }
        
        // Detect finger count based on image complexity patterns
        if (transitionRatio > 0.4 && darkRatio > 0.2 && brightRatio > 0.2) {
            // High complexity - likely multiple fingers
            if (transitionRatio > 0.6) return 5;
            if (transitionRatio > 0.5) return 4;
            if (transitionRatio > 0.45) return 3;
            return 2;
        } else if (transitionRatio > 0.2 && (darkRatio > 0.15 || brightRatio > 0.15)) {
            // Medium complexity - likely 1-2 fingers
            if (transitionRatio > 0.3) return 2;
            return 1;
        }
        
        // Low complexity but some variation - maybe 1 finger
        if (transitionRatio > 0.15) {
            return 1;
        }
        
        return 0; // No fingers detected
    }
    
    uint8_t* getImageBuffer() { return imageBuffer; }
    size_t getImageSize() { return bufferSize; }
    
    void printCameraInfo() {
        if (!isInitialized) {
            Serial.println("Camera not initialized");
            return;
        }
        
        uint8_t vid, pid;
        myCAM->wrSensorReg8_8(0xff, 0x01);
        myCAM->rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
        myCAM->rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
        
        Serial.printf("Camera ID: 0x%02X 0x%02X\n", vid, pid);
    }
    
    bool testCamera() {
        return testSPI() && testI2C();
    }
    
    bool captureJPEG(uint8_t** jpegData, size_t* jpegSize) {
        if (!isInitialized) {
            Serial.println("Camera not initialized");
            return false;
        }
        
        // Capture image
        if (!captureImage()) {
            return false;
        }
        
        // Read FIFO length
        uint32_t length = myCAM->read_fifo_length();
        Serial.printf("FIFO length: %d bytes\n", length);
        
        // Handle PSRAM board quirks
        if (length >= MAX_FIFO_SIZE || length == 0 || length > 100000) {
            Serial.printf("Invalid FIFO length detected: %d, using default\n", length);
            length = 8192; // Use reasonable default for 160x120 JPEG
        }
        
        // Allocate buffer for JPEG data
        *jpegData = (uint8_t*)malloc(length);
        if (!*jpegData) {
            Serial.println("Failed to allocate JPEG buffer");
            return false;
        }
        
        *jpegSize = length;
        
        // Read JPEG data from FIFO
        myCAM->CS_LOW();
        myCAM->set_fifo_burst();
        
        for (uint32_t i = 0; i < length; i++) {
            (*jpegData)[i] = SPI.transfer(0x00);
        }
        
        myCAM->CS_HIGH();
        
        Serial.printf("JPEG captured: %d bytes\n", *jpegSize);
        return true;
    }
};

#endif