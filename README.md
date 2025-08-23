# Pandora

Open Hardware and Open Source Home Automation Platform

## Overview

**Pandora** is an open platform for building robust home automation devices that gives users complete control over their smart home ecosystem. Built on the Zephyr RTOS and leveraging modern IoT protocols like WiFi and Thread/OpenThread with CoAP, Pandora addresses the key issues with current smart home devices: privacy concerns, cloud dependency, vendor lock-in, and planned obsolescence.

### Key Principles

- **User-repairable hardware** - Open hardware designs that users can modify and repair
- **Local processing** - No cloud dependency, all processing happens locally  
- **Open source software stack** - Complete control over firmware and functionality
- **Give back control to users** - No vendor lock-in or forced obsolescence

### Hermes Subsystem

The **Hermes subsystem** is the core IoT framework that provides:

- **Automatic Discovery**: Devices automatically discover and register with servers
- **State Management**: Comprehensive device lifecycle and state machine
- **CoAP Communication**: Both client and server functionality
- **Network Management**: Automatic connection and device provisioning
- **Persistent Settings**: Configuration survives device reboots
- **Extensible Architecture**: Easy to add new device types

## Current Status

### Working Features
- Power/Light switch devices running on Zephyr
- CoAP-based device discovery and control
- JSON API for device state management
- Multiple development platform support
- **WiFi connectivity**: ESP32/ESP32C6 with automatic provisioning
- **Thread networking**: CC1352P7 with OpenThread support
- Persistent configuration storage

### In Development
- MCUboot integration for secure OTA updates
- Enhanced security and encryption
- Matter protocol migration
- More device type support

## Quick Start

### Prerequisites
- Zephyr SDK and development environment
- Supported development board (ESP32 DevKit or CC1352P7-LP recommended)
- West tool for project management

### Building a Sample Application

```bash
# Clone the repository
git clone https://github.com/anobli/pandora.git
cd pandora

# Initialize west workspace
west init -l .
west update

# Build for WiFi-based device (ESP32)
west build -b esp32_devkitc_procpu samples/hermes/hermes_light

# Or build for Thread-based device (CC1352P7)
west build -b cc1352p7_lp samples/hermes/hermes_light

# Flash to device
west flash
```

## Device Types

### Currently Supported
- **Light Controllers**: On/off, dimming, color temperature, RGB control
- **Power Switches**: Relay-based switching modules

### Planned
- Light bulbs with integrated controllers
- Door/window sensors
- Motion detectors
- Climate sensors

## Integration

### CoAP2MQTT Bridge
For integration with MQTT-based home automation systems, use the companion [CoAP2MQTT](https://github.com/anobli/coap2mqtt) bridge.

#### Home Assistant Integration
Pandora devices integrate with Home Assistant through the CoAP2MQTT bridge:
- **CoAP to MQTT translation**: Bridge handles protocol conversion
- **Automatic device discovery** in Home Assistant via MQTT discovery
- **Standard entity types** (lights, switches, sensors)
- **Real-time state updates** with immediate response
- **Network flexibility**: Works with both WiFi and Thread networks

## Development

### Adding New Device Types

1. Create device tree bindings in `dts/bindings/`
2. Implement driver in `drivers/` if needed
3. Add device-specific service in `subsys/hermes/`
4. Create sample application in `samples/`

### Contributing

Contributions are welcome! Please see:
- **Hardware designs**: https://github.com/anobli/pandora-hw
- **CoAP2MQTT bridge**: https://github.com/anobli/coap2mqtt
- **Main firmware**: This repository

## License

This project is licensed under the Apache 2.0 License - see the individual file headers for details.

## Future Plans

- **Matter Support**: Migration from custom CoAP to Matter protocol
- **Enhanced Security**: Full encryption and secure provisioning
- **Consumer Products**: Moving beyond maker-friendly to consumer-ready devices
- **Extended Device Support**: More sensors, actuators, and device types
- **Cloud Integration**: Optional cloud services while maintaining local control

---

*Pandora: Giving users back control of their smart homes*