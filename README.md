# ESP32 SmartHome Center

## Hardware

Based on [ESP32-A1S-AudioKit](https://docs.ai-thinker.com/en/esp32-audio-kit)

- [ESP32-D0WD / ESP32-A1S](https://github.com/Ai-Thinker-Open/ESP32-A1S-AudioKit)

```
esptool.py v4.1
Serial port /dev/cu.usbserial-0001
Connecting....
Detecting chip type... Unsupported detection protocol, switching and trying again...
Connecting.....
Detecting chip type... ESP32
Chip is ESP32-D0WD (revision 1)
Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
Crystal is 40MHz
MAC: a4:e5:7c:e9:30:70
Uploading stub...
Running stub...
Stub running...
Warning: ESP32 has no Chip ID. Reading MAC instead.
MAC: a4:e5:7c:e9:30:70
Hard resetting via RTS pin...
```

## Jumper

- Jumper positions are: OFF,ON,ON,OFF,OFF

## SD-Card

- Format as exFAT
- Copy mp3 files and configuration file to the SD-Card

## Dependencies

- https://github.com/pschatzmann/arduino-audio-tools/tree/v0.8.7
- https://github.com/pschatzmann/arduino-audiokit/tree/v0.6.0
- https://github.com/pschatzmann/arduino-libhelix/tree/v0.7
- https://github.com/greiman/SdFat/tree/2.2.0
- https://github.com/knolleary/pubsubclient/tree/v2.8
