# Interactive Kinetic Sculpture That Dances with a Human

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-50%20Hz%20Control-06B6D4?style=flat-square)
![MediaPipe](https://img.shields.io/badge/MediaPipe-BlazePose%2030%20FPS-D946EF?style=flat-square)
![Telemetry](https://img.shields.io/badge/UDP-Port%205005-00FF33?style=flat-square)

**EEE4022S Final-Year Project** | Ebrahim Bhyat (`BHYEBR002`) | Supervisor: Dr. Stacey Shield | University of Cape Town

A 4-DOF dual-arm kinetic sculpture that tracks a performer without wearable markers and translates human movement into fluid robotic choreography in real time.

<p align="center">
  <img src="media/demo.gif" alt="Kinetic Sculpture Demo" width="62%" />
  <img src="media/mobile_ui.jpg" alt="ESP32 Mobile Controller" width="32%" />
</p>

## System Overview
* **Vision Host (`30.4 FPS`):** OpenCV (1-frame buffer) + MediaPipe BlazePose extract 12 normalised 2D upper-body landmarks and stream them via non-blocking **UDP (`Port 5005`)**.
* **Dual-Core ESP32-S3 (`50 Hz`):** Separates Wi-Fi, UDP parsing, and an onboard HTTP mobile dashboard from a deterministic `20 ms` hardware PWM loop, with a **`333 ms` UDP timeout Safety Hold**.
* **Virtual Damping Control:** A **`15 µs` (`2.0°`) deadband** rejects optical jitter, while an Exponential Moving Average filter ($y[n] = 0.04\,x[n] + 0.96\,y[n-1]$, transfer function $H(s) = \frac{1}{0.48s + 1}$) turns rigid hobby servos into smooth, weighted motion.

## Choreographic Modes & Web Dashboard
Controlled wirelessly via the ESP32 web UI (`http://<ESP32_IP>`), a pushbutton on `GPIO 4`, or Serial (`'m'`):
1. **Inverted Mirror (`SPACE: DIRECT`):** `1:1` right-arm mirroring with synchronous chiral inversion on the left arm.
2. **Temporal Echo (`SPACE: INVERTED`):** Left arm trails the leader by `~250–300 ms` (`12` frames) via a `30`-frame circular ring buffer into negative space.
3. **Harmonic Flux (`FLOW: FREE / BOUND`):** Maps human wrist/elbow velocity and gesture span to modulate multi-harmonic wave frequency (`0.55×–2.50×`) and amplitude.

**HTTP Endpoints:** `GET /` (dashboard UI) · `GET /status` (live JSON pulse widths) · `GET /setMode?m={0|1|2}`

## Hardware & Pinout
* **Mechanics:** Dual 2-DOF planar 3D-printed PLA arms (`200 mm` links, `400 mm` radial reach, pivot bearings) driven by `4× DS3240` servos (`500–2500 µs`).
* **Power Decoupling:** Isolated `6.8 V` servo rail buffered by a `2000 µF` capacitor bank (handling `3.20 A` peak inrush) + independent `5 V → 3.3 V` LDO regulator for the ESP32-S3.

| Component | ESP32-S3 Pin | Notes |
| :--- | :--- | :--- |
| Left / Right Shoulder | `GPIO 17` / `GPIO 18` | `50 Hz` PWM (`1500 µs` upright centre) |
| Left / Right Elbow | `GPIO 13` / `GPIO 14` | `50 Hz` PWM (`1500 µs` upright centre) |

## Key Benchmarks
* **`525 ms`** end-to-end latency (`240 fps` slow-mo; dominated by intentional `0.48 s` EMA damping)
* **`≥ 96.0%`** pose confidence down to **`3.3 Lux`** across 8 chromatic LED stage profiles
* **`82.5%`** baseline UDP packet delivery ratio · **`45%`** peak-to-peak servo jitter reduction (`1.33 V → 0.72 V`)

## Quick Start
1. **Flash ESP32-S3:** Open `firmware/Kinetic_Dance_Sculpture/Kinetic_Dance_Sculpture.ino`, install `ESP32Servo`, set your `2.4 GHz` Wi-Fi credentials (enable *Maximize Compatibility* on iPhone hotspots), and upload.
2. **Start Vision Host:** Install `mediapipe` and `opencv-python`,
