#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <WiFiManager.h>

#define FLAME_PIN 2
ESP8266WebServer server(80);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;
const int   daylightOffset_sec = 0;

bool flameDetected = false;
unsigned long lastSensorRead = 0;
const unsigned long sensorReadInterval = 200;

const int DEBOUNCE_THRESHOLD = 5;
int flameDebounceCount = 0;
bool flameState = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n启动 ESP-01S 火焰传感器");

  pinMode(FLAME_PIN, INPUT);

  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("ESP-01S_640c")) {
    Serial.println("配网超时或失败，重启...");
    ESP.restart();
    delay(1000);
  }

  Serial.println("Wi-Fi 已连接！");
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("正在同步 NTP 时间...");
  int retry = 0;
  while (time(nullptr) < 100000 && retry < 20) {
    delay(500);
    retry++;
    Serial.print(".");
  }
  if (time(nullptr) >= 100000) {
    Serial.println(" 时间同步成功！");
  } else {
    Serial.println(" 时间同步失败，请检查网络！");
  }

  server.on("/", handleRoot);
  server.on("/time", handleTime);
  server.on("/sensor", handleSensor);

  server.begin();
  Serial.println("HTTP 服务器已启动");
  Serial.print("请访问: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();

  if (millis() - lastSensorRead >= sensorReadInterval) {
    lastSensorRead = millis();
    readFlameSensor();
  }
}

void readFlameSensor() {
  int rawValue = digitalRead(FLAME_PIN);
  
  if (rawValue == LOW) {
    flameDebounceCount++;
    if (flameDebounceCount >= DEBOUNCE_THRESHOLD) {
      flameDebounceCount = DEBOUNCE_THRESHOLD;
      if (!flameState) {
        flameState = true;
        Serial.println("🔥 检测到火焰！");
      }
    }
  } else {
    flameDebounceCount--;
    if (flameDebounceCount <= 0) {
      flameDebounceCount = 0;
      if (flameState) {
        flameState = false;
        Serial.println("✅ 火焰已消失");
      }
    }
  }
  
  flameDetected = flameState;
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP-01S 火焰传感器</title>
  <style>
    body { display:flex; justify-content:center; align-items:center; height:100vh; margin:0; font-family:Arial; background:linear-gradient(135deg,#1a1a2e,#16213e,#0f3460); color:#fff; }
    .container { text-align:center; background:rgba(255,255,255,0.1); padding:2.5rem 3rem; border-radius:20px; backdrop-filter:blur(10px); border:1px solid rgba(255,255,255,0.18); min-width:320px; }
    h1 { font-size:1.5rem; font-weight:300; letter-spacing:3px; margin-bottom:1rem; opacity:0.8; }
    #clock { font-size:3.5rem; font-weight:700; letter-spacing:3px; text-shadow:0 0 20px rgba(0,150,255,0.5); font-variant-numeric: tabular-nums; }
    .ms { font-size:2rem; opacity:0.7; }
    .date { margin-top:0.2rem; font-size:1rem; opacity:0.6; }
    .sensor-card { margin-top:2rem; padding:0; border-radius:15px; transition:all 0.3s ease; }
    .sensor-card.normal { background:rgba(76,175,80,0.15); border:2px solid rgba(76,175,80,0.4); }
    .sensor-card.alert { background:rgba(244,67,54,0.15); border:2px solid rgba(244,67,54,0.5); animation:pulse 1s infinite; }
    .sensor-title { font-size:1.1rem; font-weight:bold; opacity:0.9; padding:1rem 1.5rem; border-bottom:2px solid rgba(255,255,255,0.3); text-align:left; }
    .sensor-content { padding:1.5rem; text-align:left; }
    .flame-icon { font-size:4rem; margin-bottom:0.5rem; }
    .flame-text { font-size:1.8rem; font-weight:bold; }
    .flame-desc { font-size:1rem; opacity:0.8; margin-top:0.3rem; }
    @keyframes pulse { 0% { box-shadow:0 0 0 0 rgba(244,67,54,0.7); } 70% { box-shadow:0 0 0 15px rgba(244,67,54,0); } 100% { box-shadow:0 0 0 0 rgba(244,67,54,0); } }
    .footer { margin-top:2rem; font-size:0.8rem; opacity:0.4; }
    .update-time { font-size:0.7rem; opacity:0.3; margin-top:0.5rem; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🔥 火焰监测系统</h1>
    <div id="clock">--:--:--<span class="ms">.---</span></div>
    <div class="date" id="date">----年--月--日</div>
    <div class="sensor-card normal" id="flame-status">
      <div class="sensor-title">🔥 火焰传感器</div>
      <div class="sensor-content">
        <div class="flame-icon">✅</div>
        <div class="flame-text" id="flame-text">监测正常</div>
        <div class="flame-desc" id="flame-desc">未检测到火焰</div>
      </div>
    </div>
    <div class="footer">ESP-01S · WiFi 配网</div>
    <div class="update-time" id="update-time">更新时间: --:--:--</div>
  </div>

  <script>
    function updateTime() {
      fetch('/time')
        .then(response => response.text())
        .then(data => {
          const parts = data.split('|');
          if (parts.length === 2) {
            const timeParts = parts[0].split('.');
            if (timeParts.length === 2) {
              document.getElementById('clock').innerHTML = timeParts[0] + '<span class="ms">.' + timeParts[1] + '</span>';
            } else {
              document.getElementById('clock').textContent = parts[0];
            }
            document.getElementById('date').textContent = parts[1];
          }
        })
        .catch(err => console.error('Time fetch error:', err));
    }

    function updateSensor() {
      fetch('/sensor')
        .then(response => response.text())
        .then(data => {
          const status = data.trim();
          const statusDiv = document.getElementById('flame-status');
          const textDiv = document.getElementById('flame-text');
          const descDiv = document.getElementById('flame-desc');
          const now = new Date();
          const timeStr = now.toLocaleTimeString('zh-CN', {hour12: false});
          document.getElementById('update-time').textContent = '更新时间: ' + timeStr;
          
          if (status === '1') {
            statusDiv.className = 'sensor-card alert';
            textDiv.textContent = '⚠️ 有火异常！';
            textDiv.style.color = '#ff5252';
            descDiv.textContent = '检测到火焰，请立即处理';
          } else {
            statusDiv.className = 'sensor-card normal';
            textDiv.textContent = '监测正常';
            textDiv.style.color = '#4caf50';
            descDiv.textContent = '未检测到火焰';
          }
        })
        .catch(err => console.error('Sensor fetch error:', err));
    }

    updateTime();
    updateSensor();
    setInterval(updateTime, 1000);
    setInterval(updateSensor, 500);
  </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleTime() {
  time_t now = time(nullptr);
  if (now >= 100000) {
    struct tm* info = localtime(&now);
    char timeBuf[20];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", info);
    unsigned long ms = millis() % 1000;
    char msBuf[4];
    snprintf(msBuf, sizeof(msBuf), "%03lu", ms);
    String timeStr = String(timeBuf) + "." + String(msBuf);
    char dateBuf[20];
    strftime(dateBuf, sizeof(dateBuf), "%Y年%m月%d日", info);
    String response = timeStr + "|" + String(dateBuf);
    server.send(200, "text/plain", response);
  } else {
    server.send(200, "text/plain", "--:--:--.---|----年--月--日");
  }
}

void handleSensor() {
  server.send(200, "text/plain", flameDetected ? "1" : "0");
}