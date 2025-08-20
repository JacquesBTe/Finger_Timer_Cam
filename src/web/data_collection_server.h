#ifndef DATA_COLLECTION_SERVER_H
#define DATA_COLLECTION_SERVER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "web_pages.h"

// Forward declaration to avoid circular includes
class ArduCamController;

// WiFi credentials - CHANGE THESE TO YOUR NETWORK!
#define WIFI_SSID "BENTEWIFI"        // <-- Change this to your WiFi name
#define WIFI_PASSWORD "bighouse831" // <-- Change this to your WiFi password

class DataCollectionServer {
private:
    AsyncWebServer server;
    bool serverStarted;
    ArduCamController* cameraPtr;
    
public:
    // Collection control variables
    bool isCollecting;
    bool shouldCollect;
    int currentFingers;
    int currentSample;
    int totalSamples;
    int samplesPerFingerCount;
    bool autoMode;
    int collectionDelay; // ms between samples
    unsigned long lastCollectionTime;
    String collectedData;
    
    DataCollectionServer() : server(80), serverStarted(false) {
        isCollecting = false;
        shouldCollect = false;
        currentFingers = 0;
        currentSample = 0;
        totalSamples = 0;
        samplesPerFingerCount = 20;
        autoMode = false;
        collectionDelay = 3000;
        lastCollectionTime = 0;
        collectedData = "";
        cameraPtr = nullptr;
    }
    
    bool init(ArduCamController* camera = nullptr) {
        cameraPtr = camera;
        // Connect to WiFi
        Serial.print("Connecting to WiFi");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\nWiFi connection failed!");
            return false;
        }
        
        Serial.println("");
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        setupRoutes();
        server.begin();
        serverStarted = true;
        
        return true;
    }
    
    void setupRoutes() {
        // Serve main page
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
            request->send(200, "text/html", getMainPage());
        });
        
        // API endpoints
        server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request){
            DynamicJsonDocument doc(1024);
            doc["isCollecting"] = isCollecting;
            doc["currentFingers"] = currentFingers;
            doc["currentSample"] = currentSample;
            doc["totalSamples"] = totalSamples;
            doc["samplesPerFingerCount"] = samplesPerFingerCount;
            doc["autoMode"] = autoMode;
            doc["collectionDelay"] = collectionDelay;
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        });
        
        server.on("/api/start", HTTP_POST, [this](AsyncWebServerRequest *request){
            isCollecting = true;
            shouldCollect = true;
            currentSample = 0;
            totalSamples = 0;
            collectedData = "";
            Serial.println("Data collection started via web interface");
            request->send(200, "text/plain", "Collection started");
        });
        
        server.on("/api/stop", HTTP_POST, [this](AsyncWebServerRequest *request){
            isCollecting = false;
            shouldCollect = false;
            Serial.println("Data collection stopped via web interface");
            request->send(200, "text/plain", "Collection stopped");
        });
        
        server.on("/api/collect", HTTP_POST, [this](AsyncWebServerRequest *request){
            shouldCollect = true;
            request->send(200, "text/plain", "Sample triggered");
        });
        
        server.on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest *request){
            if (request->hasParam("fingers", true)) {
                currentFingers = request->getParam("fingers", true)->value().toInt();
            }
            if (request->hasParam("samplesPerCount", true)) {
                samplesPerFingerCount = request->getParam("samplesPerCount", true)->value().toInt();
            }
            if (request->hasParam("delay", true)) {
                collectionDelay = request->getParam("delay", true)->value().toInt();
            }
            if (request->hasParam("autoMode", true)) {
                autoMode = request->getParam("autoMode", true)->value() == "true";
            }
            
            Serial.printf("Settings updated: fingers=%d, samples=%d, delay=%d, auto=%s\n", 
                         currentFingers, samplesPerFingerCount, collectionDelay, autoMode ? "true" : "false");
            request->send(200, "text/plain", "Settings updated");
        });
        
        server.on("/api/data", HTTP_GET, [this](AsyncWebServerRequest *request){
            request->send(200, "text/plain", collectedData);
        });
        
        server.on("/api/cleardata", HTTP_POST, [this](AsyncWebServerRequest *request){
            collectedData = "";
            totalSamples = 0;
            request->send(200, "text/plain", "Data cleared");
        });
        
        // Camera streaming endpoints
        server.on("/api/camera/capture", HTTP_GET, [this](AsyncWebServerRequest *request){
            if (!cameraPtr) {
                request->send(500, "text/plain", "Camera not initialized");
                return;
            }
            
            if (!cameraPtr->captureImage()) {
                request->send(500, "text/plain", "Failed to capture image");
                return;
            }
            
            // Get JPEG data from camera
            uint8_t* jpegData = nullptr;
            size_t jpegSize = 0;
            
            if (getCameraJPEG(&jpegData, &jpegSize)) {
                AsyncWebServerResponse *response = request->beginResponse_P(200, "image/jpeg", jpegData, jpegSize);
                response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
                response->addHeader("Pragma", "no-cache");
                response->addHeader("Expires", "0");
                request->send(response);
                free(jpegData);
            } else {
                request->send(500, "text/plain", "Failed to get JPEG data");
            }
        });
    }
    
    bool getCameraJPEG(uint8_t** jpegData, size_t* jpegSize) {
        if (!cameraPtr) return false;
        
        // Use the camera's built-in JPEG capture function
        return cameraPtr->captureJPEG(jpegData, jpegSize);
    }
    
    String getMainPage() {
        return String(getDataCollectionHTML());
    }
    
    
    bool isServerStarted() { return serverStarted; }
    String getIPAddress() { 
        IPAddress ip = WiFi.localIP();
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }
    
    void addDataSample(const String& sample) {
        collectedData += sample + "\n";
        totalSamples++;
    }
    
    bool shouldTriggerCollection() {
        if (!isCollecting) return false;
        
        if (autoMode) {
            return (millis() - lastCollectionTime) > collectionDelay;
        } else {
            return shouldCollect;
        }
    }
    
    void onSampleCollected() {
        shouldCollect = false;
        lastCollectionTime = millis();
        currentSample++;
        
        if (autoMode && currentSample >= samplesPerFingerCount) {
            currentFingers++;
            currentSample = 0;
            
            if (currentFingers > 5) {
                isCollecting = false;
                Serial.println("Auto collection completed for all finger counts (0-5)");
            }
        }
    }
};

#endif