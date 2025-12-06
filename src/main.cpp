#include <Arduino.h>
#include <HardwareSerial.h>
#include <ld2410.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// 1. Khai báo HardwareSerial
HardwareSerial LD2410_Serial(1);

// 2. Khai báo đối tượng cảm biến (Lớp ld2410 - viết thường)
ld2410 ld2410_sensor;

// Định nghĩa chân cho ESP32-C3
const int RX_PIN = 6;       // Nối với TX của LD2410 (U0RXD)
const int TX_PIN = 5;       // Nối với RX của LD2410 (U0TXD)
const int STATUS_LED = 8;    // Chân báo trạng thái có người
const int RELAY_PIN = 7;    // Chân điều khiển relay

// WiFi AP settings
const char* ssid = "DSS Door Opening Sensor";
const char* password = "Dss@12345678";

// Web Server
WebServer server(80);

// Sensor data
bool personDetected = false;
uint16_t movingDistance = 0;
uint16_t stationaryDistance = 0;
uint8_t movingEnergy = 0;
uint8_t stationaryEnergy = 0;
uint16_t distanceThreshold = 150; // Ngưỡng khoảng cách (cm) có thể thay đổi qua web
bool autoMode = true; // Chế độ tự động (true) hoặc thủ công (false)

// Debouncing
bool lastDoorState = false;
unsigned long lastActivationTime = 0;
const unsigned long MIN_ON_TIME = 5000; // Relay phải bật tối thiểu 5 giây trước khi tắt

// Web handlers
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  html += "<title>LD2410</title><style>body{font-family:Arial;background:#1a1a2e;color:#eee;padding:20px}";
  html += ".c{max-width:600px;margin:0 auto}h1{text-align:center;color:#4CAF50}";
  html += ".s{background:#16213e;padding:15px;border-radius:8px;margin:10px 0}";
  html += ".i{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #0f3460}";
  html += ".i:last-child{border:none}.v{color:#4CAF50;font-weight:bold}";
  html += ".d{font-size:20px;text-align:center;padding:12px;background:#16213e;border-radius:8px;margin:10px 0}";
  html += ".btn{background:#4CAF50;color:#fff;border:none;padding:12px;border-radius:5px;width:100%;font-size:16px;margin-top:10px}";
  html += ".btn:active{background:#3d8b40}";
  html += ".btn-off{background:#f44336}.btn-off:active{background:#d32f2f}";
  html += "input[type=range]{width:100%;margin:10px 0}";
  html += ".ctrl{background:#16213e;padding:15px;border-radius:8px;margin:10px 0}";
  html += ".switch{position:relative;display:inline-block;width:60px;height:34px}";
  html += ".switch input{opacity:0;width:0;height:0}";
  html += ".slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;transition:.4s;border-radius:34px}";
  html += ".slider:before{position:absolute;content:'';height:26px;width:26px;left:4px;bottom:4px;background-color:white;transition:.4s;border-radius:50%}";
  html += "input:checked+.slider{background-color:#4CAF50}";
  html += "input:checked+.slider:before{transform:translateX(26px)}";
  html += "</style></head><body><div class='c'><h1>LD2410 Door</h1>";
  html += "<div class='d' id='s'>Loading...</div><div class='s'>";
  html += "<div class='i'><span>Moving Dist:</span><span class='v' id='md'>-</span></div>";
  html += "<div class='i'><span>Moving Energy:</span><span class='v' id='me'>-</span></div>";
  html += "<div class='i'><span>Static Dist:</span><span class='v' id='sd'>-</span></div>";
  html += "<div class='i'><span>Static Energy:</span><span class='v' id='se'>-</span></div></div>";
  html += "<div class='ctrl'><div class='i'><span>Mode:</span><span class='v' id='mode'>" + String(autoMode ? "AUTO" : "MANUAL") + "</span></div>";
  html += "<label class='switch'><input type='checkbox' id='modeSwitch' " + String(autoMode ? "checked" : "") + " onchange='toggleMode(this.checked)'>";
  html += "<span class='slider'></span></label><span style='margin-left:10px;color:#888'>Auto Mode</span></div>";
  html += "<div class='ctrl'><div class='i'><span>Distance Threshold:</span><span class='v' id='th'>" + String(distanceThreshold) + " cm</span></div>";
  html += "<input type='range' min='50' max='500' value='" + String(distanceThreshold) + "' id='range' oninput='updateThreshold(this.value)'>";
  html += "<div style='display:flex;justify-content:space-between;color:#888;font-size:12px'><span>50cm</span><span>500cm</span></div></div>";
  html += "<button class='btn' id='relayBtn' onclick='manualRelay()'>Manual Relay Control</button></div>";
  html += "<script>function u(){fetch('/data').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('s').innerHTML=d.detected?'DETECTED':'NO PERSON';";
  html += "document.getElementById('s').style.color=d.detected?'#4CAF50':'#f44336';";
  html += "document.getElementById('md').textContent=d.movingDistance+' cm';";
  html += "document.getElementById('me').textContent=d.movingEnergy;";
  html += "document.getElementById('sd').textContent=d.stationaryDistance+' cm';";
  html += "document.getElementById('se').textContent=d.stationaryEnergy;";
  html += "document.getElementById('th').textContent=d.threshold+' cm';";
  html += "document.getElementById('mode').textContent=d.autoMode?'AUTO':'MANUAL';})}";
  html += "function updateThreshold(v){document.getElementById('th').textContent=v+' cm';";
  html += "fetch('/threshold?value='+v).then(r=>r.json()).then(d=>console.log(d.message));}";
  html += "function toggleMode(auto){fetch('/mode?auto='+(auto?'1':'0')).then(r=>r.json()).then(d=>{";
  html += "document.getElementById('mode').textContent=d.autoMode?'AUTO':'MANUAL';});}";
  html += "function manualRelay(){fetch('/relay').then(r=>r.json()).then(d=>alert(d.message));}";
  html += "setInterval(u,500);u();</script></body></html>";
  server.send(200, "text/html", html);
}

void handleData() {
  JsonDocument doc;
  doc["detected"] = personDetected;
  doc["movingDistance"] = movingDistance;
  doc["movingEnergy"] = movingEnergy;
  doc["stationaryDistance"] = stationaryDistance;
  doc["stationaryEnergy"] = stationaryEnergy;
  doc["threshold"] = distanceThreshold;
  doc["autoMode"] = autoMode;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleRelay() {
  static bool relayState = false;
  relayState = !relayState;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);  // HIGH = ON, LOW = OFF
  
  JsonDocument doc;
  doc["status"] = "ok";
  doc["message"] = relayState ? "Relay ON" : "Relay OFF";
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleThreshold() {
  if (server.hasArg("value")) {
    distanceThreshold = server.arg("value").toInt();
    if (distanceThreshold < 50) distanceThreshold = 50;
    if (distanceThreshold > 500) distanceThreshold = 500;
    
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Threshold set to " + String(distanceThreshold) + "cm";
    doc["threshold"] = distanceThreshold;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing value\"}");
  }
}

void handleMode() {
  if (server.hasArg("auto")) {
    autoMode = (server.arg("auto") == "1");
    
    // Nếu chuyển về chế độ AUTO, tắt relay (LOW = OFF)
    if (autoMode) {
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(STATUS_LED, HIGH);
      lastDoorState = false;
    }
    
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = autoMode ? "Auto mode enabled" : "Manual mode enabled";
    doc["autoMode"] = autoMode;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing auto parameter\"}");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("--- ESP32-C3 & LD2410 Door Control ---");

  // Khởi tạo GPIO
  pinMode(STATUS_LED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);  // LED OFF
  digitalWrite(RELAY_PIN, LOW);    // Relay OFF (LOW = OFF)

  // Khởi tạo UART cho cảm biến
  LD2410_Serial.begin(256000, SERIAL_8N1, RX_PIN, TX_PIN);

  // Kết nối cảm biến
  if (ld2410_sensor.begin(LD2410_Serial)) {
    Serial.println("LD2410 connected successfully!");
  } else {
    Serial.println("LD2410 connection failed. Check wiring.");
  }

  // Khởi tạo WiFi AP
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP started");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Khởi tạo Web Server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/relay", handleRelay);
  server.on("/threshold", handleThreshold);
  server.on("/mode", handleMode);
  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  // Xử lý web server
  server.handleClient();
  
  // Đọc dữ liệu cảm biến
  ld2410_sensor.read();
  
  // Kiểm tra sự hiện diện
  bool isMoving = ld2410_sensor.movingTargetEnergy() > 0;
  bool isStationary = ld2410_sensor.stationaryTargetEnergy() > 0;
  personDetected = isMoving || isStationary;
  
  // Cập nhật dữ liệu
  movingDistance = ld2410_sensor.movingTargetDistance();
  movingEnergy = ld2410_sensor.movingTargetEnergy();
  stationaryDistance = ld2410_sensor.stationaryTargetDistance();
  stationaryEnergy = ld2410_sensor.stationaryTargetEnergy();
  
  // Điều khiển LED và Relay
  if (autoMode) {
    // Chế độ TỰ ĐỘNG - Bật khi phát hiện chuyển động ở khoảng cách <= distanceThreshold
    bool activateDoor = (isMoving && movingDistance > 0 && movingDistance <= distanceThreshold);
    
    unsigned long currentTime = millis();
    
    // Nếu phát hiện người → Bật ngay lập tức
    if (activateDoor && !lastDoorState) {
      lastDoorState = true;
      lastActivationTime = currentTime;
      digitalWrite(STATUS_LED, LOW);
      digitalWrite(RELAY_PIN, HIGH);  // HIGH = ON
      Serial.println("Relay ON - Person detected");
    }
    // Nếu không còn phát hiện người → Chỉ tắt sau khi đã bật tối thiểu MIN_ON_TIME
    else if (!activateDoor && lastDoorState) {
      if (currentTime - lastActivationTime >= MIN_ON_TIME) {
        lastDoorState = false;
        digitalWrite(STATUS_LED, HIGH);
        digitalWrite(RELAY_PIN, LOW);  // LOW = OFF
        Serial.println("Relay OFF - Minimum time elapsed");
      }
    }
  }
  // Nếu ở chế độ MANUAL, không thay đổi trạng thái relay (chỉ điều khiển qua web)

  // In thông tin ra Serial
  if (personDetected) {
    Serial.print("Detected: ");
    
    if (isMoving) {
      Serial.print("MOVING ");
    }
    if (isStationary) {
      Serial.print("STATIONARY");
    }
    Serial.println("");

    if (isMoving) {
      Serial.print("  Moving -> Dist: ");
      Serial.print(movingDistance);
      Serial.print("cm, Energy: ");
      Serial.println(movingEnergy);
    }

    if (isStationary) {
      Serial.print("  Static -> Dist: ");
      Serial.print(stationaryDistance);
      Serial.print("cm, Energy: ");
      Serial.println(stationaryEnergy);
    }
    
    Serial.println("--------------------------------");
  }

  delay(100);
}
