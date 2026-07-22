# ESP8266 Flame Sensor Web Monitor

An ESP8266-01S based flame detection system with a real-time web interface.

## Features

- **Flame Detection** using a digital flame sensor on GPIO2 (active LOW)
- **Debounced Sensor Reading** — 5-sample debounce threshold to prevent false triggers
- **WiFiManager** — no hardcoded WiFi credentials; configure via captive portal
- **NTP Clock** — displays current time with milliseconds, synchronized via NTP (UTC+8)
- **Web Dashboard** — real-time flame status and clock display

## Hardware Wiring

| ESP8266-01S | Flame Sensor |
|-------------|--------------|
| GPIO2       | DO (Digital Output) |
| 3.3V        | VCC |
| GND         | GND |

> The flame sensor outputs **LOW** when flame is detected (active low).

## Getting Started

1. Install [Arduino IDE](https://www.arduino.cc/en/software) and ESP8266 board support.
2. Install the **WiFiManager** library via Library Manager.
3. Open `wifi_webpage1.ino`, select board **Generic ESP8266 Module**, and upload.
4. On first boot, connect to the WiFi hotspot `ESP-01S_640c` and configure your WiFi.
5. Open the displayed IP address in a browser to see the dashboard.

## Web Interface

- **Flame status** — shows "Normal" or "🔥 Fire detected!" with visual alert animation
- **Real-time clock** — displays NTP-synchronized time with milliseconds
- **Auto-refresh** — sensor updates every 500ms, clock updates every second

## API Endpoints

- `GET /` — HTML dashboard
- `GET /time` — returns `HH:MM:SS.mmm|YYYY年MM月DD日`
- `GET /sensor` — returns `1` (flame detected) or `0` (no flame)

## Dependencies

- [WiFiManager](https://github.com/tzapu/WiFiManager) by tzapu
- ESP8266WiFi (built-in)
- ESP8266WebServer (built-in)
