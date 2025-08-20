#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

// OLED Display Pins (I2C)
#define OLED_SDA        21
#define OLED_SCL        22
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_ADDRESS    0x3C

// ArduCAM Pins
#define CAM_CS          5
#define CAM_MOSI        23 //blue 
#define CAM_MISO        19 //purple
#define CAM_SCK         18
#define CAM_SDA         21  // Shared I2C //Orange wire
#define CAM_SCL         22  // Shared I2C //yellow wire

// Buzzer Pin
#define BUZZER_PIN      4

// Camera Settings
#define CAM_WIDTH       160
#define CAM_HEIGHT      120
#define FINGER_DETECT_THRESHOLD 50

// Timer Settings
#define MAX_TIMER_MINUTES 10
#define MIN_TIMER_SECONDS 5

#endif