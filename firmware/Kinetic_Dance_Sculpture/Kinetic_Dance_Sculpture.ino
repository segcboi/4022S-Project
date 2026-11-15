#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <math.h> 

// --- Wi-Fi Credentials ---
const char* ssid = "Ebs iPhone";
const char* password = "favourite";

// --- UDP Settings ---
WiFiUDP udp;
const int udpPort = 5005;  
char packetBuffer[255];    

// --- Web Server on Port 80 (Phone Dashboard Screen) ---
WebServer server(80);

// --- Servo Configuration ---
Servo leftShoulderServo;
Servo leftElbowServo;
Servo rightShoulderServo;
Servo rightElbowServo;

// --- Physical Pin Assignments ---
constexpr int PIN_L_SHOULDER = 17; 
constexpr int PIN_R_SHOULDER = 18; 

constexpr int PIN_L_ELBOW    = 13; 
constexpr int PIN_R_ELBOW    = 14; 

// Standard 270-degree DS3240 pulse limits
constexpr int SERVO_MIN_US = 500;   // 0 degrees
constexpr int SERVO_MAX_US = 2500;  // 270 degrees

// --- Fluidity & Noise Filter Variables ---
const float SMOOTHING_FACTOR = 0.04; 
const float DEADBAND_US = 15.0; 
constexpr int CENTER_US = 1500; // 135 degrees

float current_l_sh_us = CENTER_US;
float current_l_el_us = CENTER_US;
float current_r_sh_us = CENTER_US;
float current_r_el_us = CENTER_US;

float target_l_sh_us = CENTER_US;
float target_l_el_us = CENTER_US;
float target_r_sh_us = CENTER_US;
float target_r_el_us = CENTER_US;

// --- Network Test Variables ---
unsigned long packets_received = 0;
unsigned long test_start_ms = 0;
bool test_running = false;

// --- Input Coordinate Smoothing for Leader (Right Arm) ---
const float COORD_SMOOTH = 0.25;
float sm_r_sh_x = 0.5, sm_r_sh_y = 0.5;
float sm_r_el_x = 0.5, sm_r_el_y = 0.5;
float sm_r_wr_x = 0.5, sm_r_wr_y = 0.5;

// --- Pushbutton & Mode Configuration ---
constexpr int PIN_BUTTON = 4;
const unsigned long DEBOUNCE_DELAY_MS = 50;
bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
unsigned long lastDebounceTime = 0;

enum DanceMode {
  MODE_1_INVERTED_MIRROR = 0, 
  MODE_2_TEMPORAL_ECHO   = 1, 
  MODE_3_HARMONIC_WAVE   = 2  
};

int currentMode = MODE_1_INVERTED_MIRROR;

const char* getModeName(int mode) {
  switch (mode) {
    case MODE_1_INVERTED_MIRROR: return "1: INVERTED MIRROR (CV Leader + Synchronous Inverted Follower)";
    case MODE_2_TEMPORAL_ECHO:   return "2: TEMPORAL ECHO CANON (CV Leader + 300ms Delayed Wave Echo)";
    case MODE_3_HARMONIC_WAVE:   return "3: HARMONIC FLUX DANCE (Speed & Amplitude Motion Responsive)";
    default:                     return "UNKNOWN";
  }
}

// --- Temporal Canon Buffer (for Mode 2) ---
constexpr int ECHO_BUFFER_SIZE = 30; // Circular buffer (~600ms)
const int ECHO_DELAY_FRAMES = 12;    // Delay canon (~250-300ms)
float echo_buf_sh[ECHO_BUFFER_SIZE];
float echo_buf_el[ECHO_BUFFER_SIZE];
int echo_head = 0;

// --- Harmonic Wave Oscillators & Motion Response (for Mode 3) ---
float wave_phi1 = 0.0f;
float wave_phi2 = 0.0f;
float wave_phi3 = 0.0f;

// Landmark positions for instantaneous velocity computation
float prev_r_wr_x = 0.5f, prev_r_wr_y = 0.5f;
float prev_l_wr_x = 0.5f, prev_l_wr_y = 0.5f;
float prev_r_el_x = 0.5f, prev_r_el_y = 0.5f;
float prev_l_el_x = 0.5f, prev_l_el_y = 0.5f;
bool has_prev_pose = false;

// Filtered human motion metrics (0.0 to 1.0)
float filtered_person_speed = 0.0f;
float filtered_motion_extent = 0.0f;
float m3_speed_mult = 0.55f;
float m3_sh_amp = 160.0f;
float m3_el_amp = 280.0f;

// Circular buffer for tracking gesture spatial bounding box over recent frames (~0.7s)
constexpr int MOTION_HIST_SIZE = 25;
float hist_r_wr_x[MOTION_HIST_SIZE];
float hist_r_wr_y[MOTION_HIST_SIZE];
float hist_l_wr_x[MOTION_HIST_SIZE];
float hist_l_wr_y[MOTION_HIST_SIZE];
int motion_hist_head = 0;
int motion_hist_count = 0;

// --- Left Robotic Arm Inversion Configuration ---
const bool INVERT_SHOULDER = true;
const bool INVERT_ELBOW    = true;

// --- Arm Upright Reference Offsets ---
const float UPRIGHT_SH_DEG = 135.0; 
const float UPRIGHT_EL_DEG = 135.0; 
const float SH_DIR         = 1.0;   
const float EL_DIR         = 1.0;   

unsigned long lastDebugMs = 0;

void handleRoot() {
  String html = R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>ACID KINETICS // RAVE STAGE</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    
    body {
      background-color: #050408;
      background-image: 
        radial-gradient(circle at 10% 20%, rgba(255, 0, 128, 0.12) 0%, transparent 40%),
        radial-gradient(circle at 90% 80%, rgba(57, 255, 20, 0.10) 0%, transparent 45%);
      color: #e0e0e0;
      font-family: 'Courier New', Courier, monospace, monospace;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-height: 100vh;
      padding: 20px 14px 40px;
      position: relative;
      overflow-x: hidden;
    }

    body::before {
      content: " ";
      display: block;
      position: fixed;
      top: 0; left: 0; bottom: 0; right: 0;
      background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.45) 50%), linear-gradient(90deg, rgba(255,0,0,0.03), rgba(0,255,0,0.01), rgba(0,0,255,0.03));
      z-index: 99;
      background-size: 100% 3px, 6px 100%;
      pointer-events: none;
      opacity: 0.85;
    }

    .hazard-strip {
      width: 100%;
      max-width: 420px;
      height: 8px;
      background: repeating-linear-gradient(-45deg, #ffee00, #ffee00 10px, #111 10px, #111 20px);
      margin-bottom: 14px;
      border: 1px solid #ffee00;
      box-shadow: 0 0 10px rgba(255, 238, 0, 0.4);
    }

    .header-box {
      width: 100%;
      max-width: 420px;
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      margin-bottom: 12px;
      border-bottom: 2px solid #221c2e;
      padding-bottom: 8px;
    }

    h1 {
      font-size: 1.35rem;
      font-weight: 900;
      letter-spacing: 2.5px;
      color: #39ff14;
      text-transform: uppercase;
      text-shadow: 0 0 8px #39ff14, 0 0 18px rgba(57, 255, 20, 0.5);
    }

    .strobe-badge {
      font-size: 0.65rem;
      font-weight: 700;
      color: #ff007f;
      background: rgba(255, 0, 127, 0.15);
      border: 1px solid #ff007f;
      padding: 3px 8px;
      letter-spacing: 1.5px;
      text-transform: uppercase;
      animation: pulseStrobe 1.2s infinite alternate;
    }

    @keyframes pulseStrobe {
      0% { opacity: 0.5; box-shadow: 0 0 4px #ff007f; }
      100% { opacity: 1.0; box-shadow: 0 0 14px #ff007f, inset 0 0 8px #ff007f; }
    }

    .card {
      background: #0d0a14;
      border: 2px solid #ff007f;
      box-shadow: 4px 4px 0px #39ff14, 0 0 25px rgba(255, 0, 127, 0.25);
      width: 100%;
      max-width: 420px;
      padding: 18px;
      margin-bottom: 18px;
      position: relative;
    }

    .card-tag {
      font-size: 0.65rem;
      font-weight: 800;
      color: #050408;
      background: #39ff14;
      display: inline-block;
      padding: 2px 7px;
      letter-spacing: 2px;
      margin-bottom: 10px;
      text-transform: uppercase;
    }

    .active-title {
      font-size: 1.4rem;
      font-weight: 900;
      letter-spacing: 1px;
      line-height: 1.15;
      text-transform: uppercase;
      margin-bottom: 8px;
      transition: all 0.2s ease;
    }

    .active-m1 { color: #39ff14; text-shadow: 0 0 10px #39ff14; }
    .active-m2 { color: #ff007f; text-shadow: 0 0 10px #ff007f; }
    .active-m3 { color: #00f0ff; text-shadow: 0 0 10px #00f0ff; }

    .desc {
      font-size: 0.78rem;
      color: #a09cb0;
      line-height: 1.45;
      border-left: 2px solid #ff007f;
      padding-left: 10px;
      margin-top: 8px;
    }

    .eq-bar-wrap {
      display: flex;
      gap: 3px;
      align-items: flex-end;
      height: 16px;
      margin-top: 14px;
    }
    .eq-bar {
      width: 6px;
      background: #39ff14;
      border-radius: 1px;
      animation: eqBounce 0.8s infinite ease-in-out alternate;
    }
    .eq-bar:nth-child(2) { animation-duration: 0.5s; background: #ff007f; }
    .eq-bar:nth-child(3) { animation-duration: 0.9s; background: #00f0ff; }
    .eq-bar:nth-child(4) { animation-duration: 0.6s; background: #ffee00; }
    .eq-bar:nth-child(5) { animation-duration: 1.1s; background: #39ff14; }
    .eq-bar:nth-child(6) { animation-duration: 0.7s; background: #ff007f; }

    @keyframes eqBounce {
      0% { height: 3px; }
      100% { height: 16px; }
    }

    .btn-container {
      display: flex;
      flex-direction: column;
      gap: 12px;
      width: 100%;
      max-width: 420px;
    }

    .btn {
      background: #0f0c18;
      border: 2px solid #2d263b;
      color: #8f88a0;
      padding: 16px 16px;
      text-align: left;
      cursor: pointer;
      display: flex;
      justify-content: space-between;
      align-items: center;
      transition: all 0.15s ease;
      position: relative;
    }

    .btn:active {
      transform: translate(2px, 2px);
    }

    .btn-index {
      font-size: 0.7rem;
      letter-spacing: 1.5px;
      font-weight: 800;
      display: block;
      margin-bottom: 2px;
      color: #655d77;
    }

    .btn-name {
      font-size: 1.05rem;
      font-weight: 800;
      letter-spacing: 1px;
      text-transform: uppercase;
      color: #e0dce8;
      display: block;
    }

    .btn-sub {
      font-size: 0.70rem;
      color: #6b647c;
      letter-spacing: 0.5px;
      margin-top: 3px;
      display: block;
    }

    .btn-trigger {
      font-size: 0.65rem;
      font-weight: 800;
      padding: 6px 10px;
      border: 1px solid #3d3450;
      color: #6b647c;
      letter-spacing: 1px;
      text-transform: uppercase;
      background: #141020;
    }

    .btn.active-0 {
      border-color: #39ff14;
      background: #0a180c;
      box-shadow: 4px 4px 0px #39ff14, inset 0 0 15px rgba(57, 255, 20, 0.2);
    }
    .btn.active-0 .btn-index { color: #39ff14; }
    .btn.active-0 .btn-name { color: #39ff14; text-shadow: 0 0 8px #39ff14; }
    .btn.active-0 .btn-trigger { border-color: #39ff14; color: #050408; background: #39ff14; font-weight: 900; }

    .btn.active-1 {
      border-color: #ff007f;
      background: #1f0614;
      box-shadow: 4px 4px 0px #ff007f, inset 0 0 15px rgba(255, 0, 127, 0.25);
    }
    .btn.active-1 .btn-index { color: #ff007f; }
    .btn.active-1 .btn-name { color: #ff007f; text-shadow: 0 0 8px #ff007f; }
    .btn.active-1 .btn-trigger { border-color: #ff007f; color: #050408; background: #ff007f; font-weight: 900; }

    .btn.active-2 {
      border-color: #00f0ff;
      background: #06181f;
      box-shadow: 4px 4px 0px #00f0ff, inset 0 0 15px rgba(0, 240, 255, 0.25);
    }
    .btn.active-2 .btn-index { color: #00f0ff; }
    .btn.active-2 .btn-name { color: #00f0ff; text-shadow: 0 0 8px #00f0ff; }
    .btn.active-2 .btn-trigger { border-color: #00f0ff; color: #050408; background: #00f0ff; font-weight: 900; }

    .telemetry-strip {
      width: 100%;
      max-width: 420px;
      margin-top: 18px;
      border: 1px dashed #302842;
      background: #08060c;
      padding: 10px 12px;
      font-size: 0.68rem;
      color: #00f0ff;
      letter-spacing: 0.5px;
      line-height: 1.4;
      display: flex;
      flex-direction: column;
      gap: 3px;
    }
    .telemetry-title {
      font-size: 0.60rem;
      color: #6d6480;
      text-transform: uppercase;
      letter-spacing: 1.5px;
      font-weight: 700;
      display: flex;
      justify-content: space-between;
    }
  </style>
</head>
<body>
  <div class="hazard-strip"></div>
  
  <div class="header-box">
    <h1>ACID KINETICS</h1>
    <div class="strobe-badge">LIVE // STAGE_V2</div>
  </div>

  <div class="card" id="card-box">
    <div class="card-tag">// CURRENT_STATE</div>
    <div id="active-title" class="active-title active-m1">MODE 1: INVERTED MIRROR</div>
    <div id="desc" class="desc">CV LEADER (R) + DIRECT CHIRAL INVERSION (L). Both arms locked vertically upright at baseline.</div>
    <div class="eq-bar-wrap">
      <div class="eq-bar"></div><div class="eq-bar"></div><div class="eq-bar"></div>
      <div class="eq-bar"></div><div class="eq-bar"></div><div class="eq-bar"></div>
    </div>
  </div>

  <div class="btn-container">
    <button class="btn active-0" id="btn-0" onclick="setMode(0)">
      <div>
        <span class="btn-index">[STAGE_01]</span>
        <span class="btn-name">INVERTED MIRROR</span>
        <span class="btn-sub">Synchronous Anti-Phase Chiral Motion</span>
      </div>
      <div class="btn-trigger" id="trig-0">LIVE</div>
    </button>

    <button class="btn" id="btn-1" onclick="setMode(1)">
      <div>
        <span class="btn-index">[STAGE_02]</span>
        <span class="btn-name">TEMPORAL ECHO</span>
        <span class="btn-sub">300ms Delayed Ring-Buffer Wave Echo</span>
      </div>
      <div class="btn-trigger" id="trig-1">DROP</div>
    </button>

    <button class="btn" id="btn-2" onclick="setMode(2)">
      <div>
        <span class="btn-index">[STAGE_03]</span>
        <span class="btn-name">HARMONIC RAVE FLUX</span>
        <span class="btn-sub">Speed & Amplitude Responsive Wave Dance</span>
      </div>
      <div class="btn-trigger" id="trig-2">FLOW</div>
    </button>
  </div>

  <div class="telemetry-strip">
    <div class="telemetry-title">
      <span>TELEMETRY_BUS :: UDP 5005</span>
      <span id="sync-dot">● ONLINE</span>
    </div>
    <div id="telem">SYNCING DATA STREAM...</div>
  </div>

  <script>
    const modes = [
      { 
        title: "MODE 1: INVERTED MIRROR", 
        desc: "CV LEADER (R) + DIRECT CHIRAL INVERSION (L). Both arms locked vertically upright at baseline.",
        cls: "active-m1",
        borderColor: "#39ff14"
      },
      { 
        title: "MODE 2: TEMPORAL ECHO", 
        desc: "CV LEADER (R) + 300MS CANON WAVE DELAY (L). Cascading echo trailing the leader.",
        cls: "active-m2",
        borderColor: "#ff007f"
      },
      { 
        title: "MODE 3: HARMONIC RAVE FLUX", 
        desc: "MOTION RESPONSIVE FLUX. Wave speed accelerates with human velocity; wave amplitude expands outward with bigger motions and tightens with subtle gestures.",
        cls: "active-m3",
        borderColor: "#00f0ff"
      }
    ];

    function setMode(idx) {
      fetch('/setMode?m=' + idx).then(r => r.json()).then(updateUI);
    }

    function updateUI(data) {
      const m = data.mode;
      const titleElem = document.getElementById('active-title');
      const cardBox = document.getElementById('card-box');
      
      titleElem.className = 'active-title ' + modes[m].cls;
      titleElem.innerText = modes[m].title;
      document.getElementById('desc').innerText = modes[m].desc;
      cardBox.style.borderColor = modes[m].borderColor;

      for (let i = 0; i < 3; i++) {
        const btn = document.getElementById('btn-' + i);
        const trig = document.getElementById('trig-' + i);
        btn.className = 'btn' + (i === m ? ' active-' + i : '');
        trig.innerText = (i === m ? 'LIVE' : (i === 1 ? 'DROP' : (i === 2 ? 'FLOW' : 'SELECT')));
      }

      document.getElementById('telem').innerText = 
        `R[sh:${data.r_sh}us el:${data.r_el}us] | L[sh:${data.l_sh}us el:${data.l_el}us]`;
    }

    setInterval(() => {
      fetch('/status').then(r => r.json()).then(updateUI).catch(() => {
        document.getElementById('sync-dot').innerText = '○ OFFLINE';
        document.getElementById('sync-dot').style.color = '#ff0055';
      });
    }, 800);
  </script>
</body>
</html>)rawliteral";
  server.send(200, "text/html", html);
}

void handleStatus() {
  String json = "{";
  json += "\"mode\":" + String(currentMode) + ",";
  json += "\"r_sh\":" + String((int)current_r_sh_us) + ",";
  json += "\"r_el\":" + String((int)current_r_el_us) + ",";
  json += "\"l_sh\":" + String((int)current_l_sh_us) + ",";
  json += "\"l_el\":" + String((int)current_l_el_us);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSetMode() {
  if (server.hasArg("m")) {
    int m = server.arg("m").toInt();
    if (m >= 0 && m <= 2) {
      currentMode = m;
      Serial.println();
      Serial.println("**************************************************");
      Serial.printf(" [WEB DASHBOARD] Switched to %s\n", getModeName(currentMode));
      Serial.println("**************************************************");
    }
  }
  handleStatus();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  for (int i = 0; i < ECHO_BUFFER_SIZE; i++) {
    echo_buf_sh[i] = CENTER_US;
    echo_buf_el[i] = CENTER_US;
  }

  for (int i = 0; i < MOTION_HIST_SIZE; i++) {
    hist_r_wr_x[i] = 0.5f;
    hist_r_wr_y[i] = 0.5f;
    hist_l_wr_x[i] = 0.5f;
    hist_l_wr_y[i] = 0.5f;
  }

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  leftShoulderServo.setPeriodHertz(50);
  leftElbowServo.setPeriodHertz(50);
  rightShoulderServo.setPeriodHertz(50);
  rightElbowServo.setPeriodHertz(50);
  
  leftShoulderServo.attach(PIN_L_SHOULDER, SERVO_MIN_US, SERVO_MAX_US);
  leftElbowServo.attach(PIN_L_ELBOW, SERVO_MIN_US, SERVO_MAX_US);
  rightShoulderServo.attach(PIN_R_SHOULDER, SERVO_MIN_US, SERVO_MAX_US);
  rightElbowServo.attach(PIN_R_ELBOW, SERVO_MIN_US, SERVO_MAX_US);
  
  leftShoulderServo.writeMicroseconds(CENTER_US);
  leftElbowServo.writeMicroseconds(CENTER_US);
  rightShoulderServo.writeMicroseconds(CENTER_US);
  rightElbowServo.writeMicroseconds(CENTER_US);

  Serial.println("\n==================================================");
  Serial.println("  ESP32 KINETIC DANCE SCULPTURE ACTIVE");
  Serial.println("==================================================");

  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/setMode", HTTP_GET, handleSetMode);
  server.begin();

  Serial.println("\n>>> PHONE WEB DASHBOARD READY! <<<");
  Serial.print("Open this URL on your phone's browser: http://");
  Serial.println(WiFi.localIP());
  Serial.println("==================================================\n");

  udp.begin(udpPort);
}

float degreesToMicroseconds(float degrees) {
  return ((degrees / 270.0) * 2000.0) + 500.0;
}

void cycleDanceMode() {
  currentMode = (currentMode + 1) % 3;
  Serial.println();
  Serial.println("**************************************************");
  Serial.printf(" [MODE SWITCHED] -> Mode %s\n", getModeName(currentMode));
  Serial.println("**************************************************");
}

void handleButtonAndSerial() {
  bool reading = digitalRead(PIN_BUTTON);
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != stableButtonState) {
      stableButtonState = reading;
      if (stableButtonState == LOW) {
        cycleDanceMode();
      }
    }
  }
  lastButtonReading = reading;

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'm' || c == 'M' || c == ' ') {
      cycleDanceMode();
    }
  }
}

void loop() {
  server.handleClient();
  handleButtonAndSerial();

  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;

    float l_sh_x, l_sh_y, l_el_x, l_el_y, l_wr_x, l_wr_y;
    float r_sh_x, r_sh_y, r_el_x, r_el_y, r_wr_x, r_wr_y;

    int parsed = sscanf(packetBuffer, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", 
                        &l_sh_x, &l_sh_y, &l_el_x, &l_el_y, &l_wr_x, &l_wr_y,
                        &r_sh_x, &r_sh_y, &r_el_x, &r_el_y, &r_wr_x, &r_wr_y);

    if (parsed == 12) {
      // 1. Update Human Motion Speed & Gesture Amplitude for Mode 3
      if (!has_prev_pose) {
        prev_r_wr_x = r_wr_x; prev_r_wr_y = r_wr_y;
        prev_l_wr_x = l_wr_x; prev_l_wr_y = l_wr_y;
        prev_r_el_x = r_el_x; prev_r_el_y = r_el_y;
        prev_l_el_x = l_el_x; prev_l_el_y = l_el_y;
        has_prev_pose = true;
      } else {
        float dr_wr = hypot(r_wr_x - prev_r_wr_x, r_wr_y - prev_r_wr_y);
        float dl_wr = hypot(l_wr_x - prev_l_wr_x, l_wr_y - prev_l_wr_y);
        float dr_el = hypot(r_el_x - prev_r_el_x, r_el_y - prev_r_el_y);
        float dl_el = hypot(l_el_x - prev_l_el_x, l_el_y - prev_l_el_y);

        // Instantaneous speed of the fastest moving arm
        float raw_speed = max(dr_wr, dl_wr) + 0.5f * max(dr_el, dl_el);

        prev_r_wr_x = r_wr_x; prev_r_wr_y = r_wr_y;
        prev_l_wr_x = l_wr_x; prev_l_wr_y = l_wr_y;
        prev_r_el_x = r_el_x; prev_r_el_y = r_el_y;
        prev_l_el_x = l_el_x; prev_l_el_y = l_el_y;

        // Asymmetric attack/release smoothing for speed
        if (raw_speed > filtered_person_speed) {
          filtered_person_speed += (raw_speed - filtered_person_speed) * 0.35f;
        } else {
          filtered_person_speed += (raw_speed - filtered_person_speed) * 0.07f;
        }
      }

      // Record wrist history for gesture span estimation
      hist_r_wr_x[motion_hist_head] = r_wr_x;
      hist_r_wr_y[motion_hist_head] = r_wr_y;
      hist_l_wr_x[motion_hist_head] = l_wr_x;
      hist_l_wr_y[motion_hist_head] = l_wr_y;
      motion_hist_head = (motion_hist_head + 1) % MOTION_HIST_SIZE;
      if (motion_hist_count < MOTION_HIST_SIZE) motion_hist_count++;

      float min_rx = 1.0f, max_rx = 0.0f, min_ry = 1.0f, max_ry = 0.0f;
      float min_lx = 1.0f, max_lx = 0.0f, min_ly = 1.0f, max_ly = 0.0f;
      for (int i = 0; i < motion_hist_count; i++) {
        if (hist_r_wr_x[i] < min_rx) min_rx = hist_r_wr_x[i];
        if (hist_r_wr_x[i] > max_rx) max_rx = hist_r_wr_x[i];
        if (hist_r_wr_y[i] < min_ry) min_ry = hist_r_wr_y[i];
        if (hist_r_wr_y[i] > max_ry) max_ry = hist_r_wr_y[i];

        if (hist_l_wr_x[i] < min_lx) min_lx = hist_l_wr_x[i];
        if (hist_l_wr_x[i] > max_lx) max_lx = hist_l_wr_x[i];
        if (hist_l_wr_y[i] < min_ly) min_ly = hist_l_wr_y[i];
        if (hist_l_wr_y[i] > max_ly) max_ly = hist_l_wr_y[i];
      }

      float stroke_r = hypot(max_rx - min_rx, (max_ry - min_ry) * 0.6f);
      float stroke_l = hypot(max_lx - min_lx, (max_ly - min_ly) * 0.6f);
      float max_stroke = max(stroke_r, stroke_l);

      // Arm reach outward from shoulder
      float reach_r = hypot(r_wr_x - r_sh_x, r_wr_y - r_sh_y);
      float reach_l = hypot(l_wr_x - l_sh_x, l_wr_y - l_sh_y);
      float reach_extent = (max(reach_r, reach_l) - 0.20f) * 1.8f;

      // 2. Standard Right Arm Kinematics for Leader Modes (Mode 1 & 2)
      sm_r_sh_x += (r_sh_x - sm_r_sh_x) * COORD_SMOOTH;
      sm_r_sh_y += (r_sh_y - sm_r_sh_y) * COORD_SMOOTH;
      sm_r_el_x += (r_el_x - sm_r_el_x) * COORD_SMOOTH;
      sm_r_el_y += (r_el_y - sm_r_el_y) * COORD_SMOOTH;
      sm_r_wr_x += (r_wr_x - sm_r_wr_x) * COORD_SMOOTH;
      sm_r_wr_y += (r_wr_y - sm_r_wr_y) * COORD_SMOOTH;

      float r_arm_dx = sm_r_el_x - sm_r_sh_x;
      float r_arm_dy = sm_r_el_y - sm_r_sh_y; 
      float human_r_sh_deg = atan2(r_arm_dy, r_arm_dx) * 180.0 / PI; 
      
      float r_forearm_dx = sm_r_wr_x - sm_r_el_x;
      float r_forearm_dy = sm_r_wr_y - sm_r_el_y;
      float human_r_el_deg = atan2(r_forearm_dy, r_forearm_dx) * 180.0 / PI;

      float elbow_flex_deg = human_r_el_deg - human_r_sh_deg;
      while (elbow_flex_deg > 180.0) elbow_flex_deg -= 360.0;
      while (elbow_flex_deg < -180.0) elbow_flex_deg += 360.0;

      float servo_r_sh_deg = constrain(SH_DIR * (human_r_sh_deg - 90.0) + UPRIGHT_SH_DEG, 20.0, 250.0);
      float servo_r_el_deg = constrain(EL_DIR * elbow_flex_deg + UPRIGHT_EL_DEG, 0.0, 270.0);

      float new_r_sh_us = degreesToMicroseconds(servo_r_sh_deg);
      float new_r_el_us = degreesToMicroseconds(servo_r_el_deg);

      // Angular excursion factor
      float ang_excursion = abs(new_r_sh_us - 1500.0f) / 600.0f;

      // Combined instantaneous motion extent
      float raw_extent = max(max_stroke * 1.6f, max(ang_excursion, reach_extent));
      raw_extent = constrain(raw_extent, 0.0f, 1.0f);

      if (raw_extent > filtered_motion_extent) {
        filtered_motion_extent += (raw_extent - filtered_motion_extent) * 0.20f;
      } else {
        filtered_motion_extent += (raw_extent - filtered_motion_extent) * 0.04f;
      }

      if (abs(new_r_sh_us - target_r_sh_us) > DEADBAND_US) { target_r_sh_us = new_r_sh_us; }
      if (abs(new_r_el_us - target_r_el_us) > DEADBAND_US) { target_r_el_us = new_r_el_us; }

      echo_buf_sh[echo_head] = target_r_sh_us;
      echo_buf_el[echo_head] = target_r_el_us;

      switch (currentMode) {
        case MODE_1_INVERTED_MIRROR: {
          target_l_sh_us = INVERT_SHOULDER ? (3000.0f - target_r_sh_us) : target_r_sh_us;
          target_l_el_us = INVERT_ELBOW    ? (3000.0f - target_r_el_us) : target_r_el_us;
          break;
        }

        case MODE_2_TEMPORAL_ECHO: {
          int read_idx = (echo_head - ECHO_DELAY_FRAMES + ECHO_BUFFER_SIZE) % ECHO_BUFFER_SIZE;
          float delayed_r_sh = echo_buf_sh[read_idx];
          float delayed_r_el = echo_buf_el[read_idx];
          target_l_sh_us = INVERT_SHOULDER ? (3000.0f - delayed_r_sh) : delayed_r_sh;
          target_l_el_us = INVERT_ELBOW    ? (3000.0f - delayed_r_el) : delayed_r_el;
          break;
        }

        case MODE_3_HARMONIC_WAVE: {
          // Handled uniformly below in the dedicated Mode 3 synthesizer block
          break;
        }
      }

      echo_head = (echo_head + 1) % ECHO_BUFFER_SIZE;
    }
  } else {
    // If no UDP packet arrived in this loop cycle, smoothly decay motion metrics
    filtered_person_speed *= 0.97f;
    filtered_motion_extent *= 0.98f;
  }

  // --- Dedicated Mode 3 Harmonic Flux Synthesizer ---
  // Runs consistently at the 50Hz control loop rate for silky fluid motion
  float active_smoothing = SMOOTHING_FACTOR;

  if (currentMode == MODE_3_HARMONIC_WAVE) {
    // 1. Responds to how fast a person moves:
    // Standing still / calm baseline is now faster (0.55x), accelerating up to 2.50x on fast movement
    float speed_norm = constrain((filtered_person_speed - 0.005f) / 0.065f, 0.0f, 1.0f);
    m3_speed_mult = 0.55f + 1.95f * speed_norm;

    wave_phi1 += 0.040f * m3_speed_mult;
    wave_phi2 += 0.056f * m3_speed_mult;
    wave_phi3 += 0.063f * m3_speed_mult;

    if (wave_phi1 > 628.3185f) wave_phi1 -= 628.3185f;
    if (wave_phi2 > 628.3185f) wave_phi2 -= 628.3185f;
    if (wave_phi3 > 628.3185f) wave_phi3 -= 628.3185f;

    // 2. Gesture extent controls range of motion:
    // Top arm (elbow) bends out significantly more for a pronounced, fluid wavy motion
    float motion_norm = constrain((filtered_motion_extent - 0.08f) / 0.60f, 0.0f, 1.0f);

    m3_sh_amp = 160.0f + (520.0f - 160.0f) * motion_norm;
    m3_el_amp = 280.0f + (600.0f - 280.0f) * motion_norm; // Top arm bends out wider (280us baseline -> 600us peak)

    float sh_wave = m3_sh_amp * (0.65f * sin(wave_phi1) + 0.35f * cos(wave_phi3));
    float el_wave = m3_el_amp * (0.60f * cos(wave_phi1 * 1.3f) + 0.40f * sin(wave_phi2));

    target_r_sh_us = constrain(1500.0f + sh_wave, (float)SERVO_MIN_US, (float)SERVO_MAX_US);
    target_r_el_us = constrain(1500.0f + el_wave, (float)SERVO_MIN_US, (float)SERVO_MAX_US);
    target_l_sh_us = constrain(1500.0f - sh_wave, (float)SERVO_MIN_US, (float)SERVO_MAX_US);
    target_l_el_us = constrain(1500.0f - el_wave, (float)SERVO_MIN_US, (float)SERVO_MAX_US);

    // Adapt smoothing factor so faster waves aren't attenuated by low-pass filter
    active_smoothing = 0.05f + 0.07f * speed_norm;
  }

  target_l_sh_us = constrain(target_l_sh_us, (float)SERVO_MIN_US, (float)SERVO_MAX_US);
  target_l_el_us = constrain(target_l_el_us, (float)SERVO_MIN_US, (float)SERVO_MAX_US);

  if (millis() - lastDebugMs > 500) {
    lastDebugMs = millis();
    if (currentMode == MODE_3_HARMONIC_WAVE) {
      Serial.printf("[M3 FLUX] SpdMult: %.2fx | Amp sh: %dus el(top): %dus | R[sh:%d el:%d] L[sh:%d el:%d]\n",
                    m3_speed_mult, (int)m3_sh_amp, (int)m3_el_amp,
                    (int)target_r_sh_us, (int)target_r_el_us,
                    (int)target_l_sh_us, (int)target_l_el_us);
    } else {
      Serial.printf("[M%d] R: sh=%d el=%d | L: sh=%d el=%d\n",
                    currentMode + 1,
                    (int)target_r_sh_us, (int)target_r_el_us,
                    (int)target_l_sh_us, (int)target_l_el_us);
    }
  }

  current_r_sh_us += (target_r_sh_us - current_r_sh_us) * active_smoothing;
  current_l_sh_us += (target_l_sh_us - current_l_sh_us) * active_smoothing;
  current_r_el_us += (target_r_el_us - current_r_el_us) * active_smoothing;
  current_l_el_us += (target_l_el_us - current_l_el_us) * active_smoothing;

  rightShoulderServo.writeMicroseconds(round(current_r_sh_us));
  leftShoulderServo.writeMicroseconds(round(current_l_sh_us));
  rightElbowServo.writeMicroseconds(round(current_r_el_us));
  leftElbowServo.writeMicroseconds(round(current_l_el_us));

  delay(20);
}
