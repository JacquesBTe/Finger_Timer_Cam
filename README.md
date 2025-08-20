# ML-Powered Finger Timer

ML-powered finger timer using Arduino 5MP camera and Edge Impulse. Show 1-5 fingers to set timer minutes, runs countdown with OLED display. Computer vision detects hand gestures for hands-free timing control. Perfect for cooking, workouts, or any timing needs. Built with PlatformIO.

## Features

- **Gesture-Based Timer Setting**: Show 1-5 fingers to set timer duration in minutes
- **ML-Powered Recognition**: Uses Edge Impulse machine learning for finger detection
- **OLED Display**: Real-time countdown and status display
- **Hands-Free Operation**: Perfect for cooking, workouts, or when your hands are busy
- **Arduino Compatible**: Built for Arduino with 5MP camera module

## Hardware Requirements

- Arduino board (ESP32 recommended)
- 5MP camera module (Arduino compatible)
- OLED display
- Connecting wires and breadboard

## Setup

1. Clone this repository
2. Open in PlatformIO
3. Install required dependencies
4. Upload to your Arduino board

## ML Model Training Tips

For best results when training your own model:

- **Use a better quality camera** if possible for improved accuracy
- **Collect 20+ samples per finger count** during data collection phase
- Ensure varied lighting conditions in your training data
- Include different hand positions and angles

## Usage

1. Power on the device
2. Show 1-5 fingers to the camera to set timer minutes
3. Timer starts automatically after gesture detection
4. Watch the countdown on the OLED display
5. Device alerts when timer completes

## Configuration

Timer settings can be adjusted in `src/config_pins.h`:
- `MAX_TIMER_MINUTES`: Maximum timer duration
- `MIN_TIMER_SECONDS`: Minimum timer duration

## Built With

- PlatformIO
- Edge Impulse ML framework
- Arduino libraries for camera and display

## License

MIT License - see LICENSE file for details