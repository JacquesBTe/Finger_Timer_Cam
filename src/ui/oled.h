#ifndef OLED_H
#define OLED_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../config_pins.h"

class OLEDDisplay {
private:
    Adafruit_SSD1306 display;
    
public:
    OLEDDisplay() : display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1) {}
    
    bool init() {
        if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
            Serial.println("SSD1306 allocation failed");
            return false;
        }
        
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Initializing...");
        display.display();
        
        Serial.println("OLED initialized");
        return true;
    }
    
    void clear() {
        display.clearDisplay();
    }
    
    void showMessage(const char* title, const char* message = "") {
        clear();
        
        // Title
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.println(title);
        
        // Message
        if (message && strlen(message) > 0) {
            display.setTextSize(1);
            display.setCursor(0, 25);
            display.println(message);
        }
        
        update();
    }
    
    void showTimer(int minutes, int seconds) {
        clear();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("Timer Set:");
        
        display.setTextSize(3);
        display.setCursor(10, 20);
        if (minutes < 10) display.print("0");
        display.print(minutes);
        display.print(":");
        if (seconds < 10) display.print("0");
        display.print(seconds);
        
        update();
    }
    
    void showFingerCount(int count) {
        clear();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("ML Detection:");
        
        display.setTextSize(4);
        display.setCursor(45, 20);
        display.print(count);
        
        // Show finger word
        display.setTextSize(1);
        display.setCursor(0, 55);
        if (count == 1) {
            display.print("1 finger");
        } else {
            display.print(String(count) + " fingers");
        }
        
        update();
    }
    
    void showMLStatus(const char* status, float confidence = 0.0) {
        clear();
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("ML Status:");
        
        display.setTextSize(1);
        display.setCursor(0, 15);
        display.println(status);
        
        if (confidence > 0) {
            display.setCursor(0, 35);
            display.print("Confidence: ");
            display.print(confidence * 100, 1);
            display.println("%");
        }
        
        update();
    }
    
    void showCountdown(int totalSeconds, int remaining) {
        clear();
        
        int minutes = remaining / 60;
        int seconds = remaining % 60;
        
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("Timer Running");
        
        display.setTextSize(3);
        display.setCursor(10, 20);
        if (minutes < 10) display.print("0");
        display.print(minutes);
        display.print(":");
        if (seconds < 10) display.print("0");
        display.print(seconds);
        
        // Progress bar
        int progress = map(remaining, 0, totalSeconds, 0, OLED_WIDTH - 4);
        display.drawRect(2, 55, OLED_WIDTH - 4, 6, SSD1306_WHITE);
        display.fillRect(2, 55, progress, 6, SSD1306_WHITE);
        
        update();
    }
    
    void showFinished() {
        clear();
        
        display.setTextSize(2);
        display.setCursor(20, 15);
        display.println("TIME'S");
        display.setCursor(30, 35);
        display.println("UP!");
        
        update();
    }
    
    void update() {
        display.display();
    }
};

#endif