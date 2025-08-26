# Smart MediBox

An intelligent pharmaceutical storage system that revolutionizes medication management through IoT technology and environmental monitoring.

## Overview

The Smart MediBox is an embedded system designed to efficiently manage medications with advanced features including automatic time synchronization, smart alarm management, environmental monitoring, and remote control capabilities. The system ensures medications are stored in optimal conditions while providing timely reminders to users.

## Video Explanations
https://drive.google.com/drive/folders/1QRBbNZgGLt7ZVYmZwcyW9U6h62nVZr1o?usp=sharing

## Key Features

### Core Functionality
- **Smart Time Management**: Automatic NTP time synchronization with configurable time zones
- **Intelligent Alarms**: Customizable medication reminders with audio-visual alerts
- **Real-time Display**: OLED screen showing current time, alerts, and system status
- **Environmental Monitoring**: Continuous temperature and humidity tracking with warnings
- **Light Control**: Automatic shaded window adjustment based on ambient light levels
- **User Interface**: Intuitive menu-driven navigation via push buttons

### Advanced Features
- **IoT Dashboard**: Real-time data visualization through Node-RED
- **Remote Control**: MQTT-based communication for remote monitoring and control
- **Persistent Storage**: Non-volatile memory for settings and alarm configurations
- **Power Efficient**: Smart change detection to minimize power consumption
- **Continuous Monitoring**: 24/7 environmental condition tracking

## System Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  User Interface │    │Alarm Management │    │ Time Management │
│    (OLED + BTN) │    │   (Buzzer + UI) │    │   (NTP + TZ)    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
         ┌─────────────────────────────────────────────────────┐
         │            Hardware Abstraction Layer               │
         └─────────────────────────────────────────────────────┘
                                 │
         ┌─────────────────────────────────────────────────────┐
         │              ESP32 Controller                       │
         └─────────────────────────────────────────────────────┘
                                 │
    ┌────────────┬────────────────┼────────────────┬────────────┐
    │            │                │                │            │
┌───▼───┐   ┌───▼───┐       ┌───▼───┐       ┌───▼───┐   ┌───▼───┐
│ DHT22 │   │  LDRs │       │ Servo │       │ MQTT  │   │ EEPROM│
│Sensor │   │(Light)│       │ Motor │       │Broker │   │Storage│
└───────┘   └───────┘       └───────┘       └───────┘   └───────┘
```

## Hardware Requirements

| Component | Purpose | Quantity |
|-----------|---------|----------|
| ESP32 Development Board | Main controller with WiFi/Bluetooth | 1 |
| OLED Display (128x64) | User interface and information display | 1 |
| Buzzer | Audio alerts for medication reminders | 1 |
| Push Buttons | User input and menu navigation | 3-4 |
| DHT22 Sensor | Temperature and humidity monitoring | 1 |
| Light Dependent Resistors | Ambient light measurement | 2 |
| Servo Motor (SG90) | Automated shaded window control | 1 |
| Resistors & Jumper Wires | Circuit connections | As needed |

## Software Requirements

- **Development Environment**: Arduino IDE or PlatformIO
- **Simulation**: Wokwi (for testing without hardware)
- **Dashboard**: Node-RED
- **Message Broker**: MQTT Broker (Mosquitto recommended)
- **Libraries**: 
  - WiFi management
  - OLED display drivers
  - DHT sensor library
  - Servo motor control
  - MQTT client
  - NTP time synchronization

## Dashboard Features

The Node-RED dashboard provides:
- Real-time temperature and humidity graphs
- Light intensity monitoring
- Remote servo motor control
- Alarm status indicators
- Historical data logging
- Mobile-responsive interface

## Usage

### Setting Up Alarms
1. Navigate to "Set Alarm" using push buttons
2. Configure time, days, and medication details
3. Save settings (stored in non-volatile memory)

### Environmental Monitoring
- Continuous temperature/humidity tracking
- Automatic warnings for unsafe conditions
- Data logging to MQTT topics

### Light Management
- Automatic shaded window adjustment
- Manual override via dashboard
- Optimal medication storage conditions

## Future Enhancements

- [ ] Mobile app integration
- [ ] Voice control with Alexa/Google Assistant
- [ ] Machine learning for usage pattern analysis
- [ ] Multi-medication compartment support
- [ ] Integration with pharmacy systems
- [ ] Advanced security features


