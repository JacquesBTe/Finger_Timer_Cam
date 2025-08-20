#ifndef DOWNSCALE_H
#define DOWNSCALE_H

#include <Arduino.h>

class ImageProcessor {
public:
    // Convert JPEG to grayscale and downscale
    static bool jpegToGrayscale(uint8_t* jpegData, size_t jpegSize, 
                               uint8_t* grayBuffer, int targetWidth, int targetHeight) {
        // Placeholder implementation
        // In a real implementation, you'd decode JPEG and convert to grayscale
        Serial.println("JPEG to grayscale conversion (placeholder)");
        return true;
    }
    
    // Simple finger detection using edge detection
    static int detectFingers(uint8_t* grayImage, int width, int height) {
        // Placeholder implementation
        // Count bright regions that could be fingers
        int fingerCount = 0;
        int brightPixels = 0;
        
        for (int i = 0; i < width * height; i++) {
            if (grayImage[i] > 128) {
                brightPixels++;
            }
        }
        
        // Simple heuristic based on bright pixel count
        fingerCount = (brightPixels / (width * height / 10)) % 6;
        return fingerCount;
    }
    
    // Threshold and edge detection helpers
    static void applyThreshold(uint8_t* image, int width, int height, uint8_t threshold) {
        for (int i = 0; i < width * height; i++) {
            image[i] = (image[i] > threshold) ? 255 : 0;
        }
    }
    
    static void sobelEdgeDetection(uint8_t* input, uint8_t* output, int width, int height) {
        // Simple Sobel edge detection
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                int gx = 0, gy = 0;
                
                // Sobel X kernel
                gx += -1 * input[(y-1)*width + (x-1)];
                gx += -2 * input[y*width + (x-1)];
                gx += -1 * input[(y+1)*width + (x-1)];
                gx += 1 * input[(y-1)*width + (x+1)];
                gx += 2 * input[y*width + (x+1)];
                gx += 1 * input[(y+1)*width + (x+1)];
                
                // Sobel Y kernel
                gy += -1 * input[(y-1)*width + (x-1)];
                gy += -2 * input[(y-1)*width + x];
                gy += -1 * input[(y-1)*width + (x+1)];
                gy += 1 * input[(y+1)*width + (x-1)];
                gy += 2 * input[(y+1)*width + x];
                gy += 1 * input[(y+1)*width + (x+1)];
                
                int magnitude = sqrt(gx*gx + gy*gy);
                output[y*width + x] = min(255, magnitude);
            }
        }
    }
    
    static int countConnectedComponents(uint8_t* binary, int width, int height) {
        // Simple connected component counting
        uint8_t* visited = new uint8_t[width * height];
        memset(visited, 0, width * height);
        
        int components = 0;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (binary[y*width + x] > 0 && !visited[y*width + x]) {
                    floodFill(visited, width, height, x, y, 1);
                    components++;
                }
            }
        }
        
        delete[] visited;
        return components;
    }
    
private:
    static void floodFill(uint8_t* image, int width, int height, int x, int y, uint8_t newVal) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        if (image[y*width + x] != 0) return;
        
        image[y*width + x] = newVal;
        
        floodFill(image, width, height, x+1, y, newVal);
        floodFill(image, width, height, x-1, y, newVal);
        floodFill(image, width, height, x, y+1, newVal);
        floodFill(image, width, height, x, y-1, newVal);
    }
};

#endif