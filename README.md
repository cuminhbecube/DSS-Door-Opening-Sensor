# DSS Door Opening Sensor

Hệ thống cảm biến mở cửa tự động sử dụng ESP32-C3 và radar LD2410 với giao diện web điều khiển.

## 🔧 Phần Cứng

- **ESP32-C3 DevKit M-1** - Vi điều khiển chính
- **LD2410 Radar Sensor** - Cảm biến radar phát hiện chuyển động
- **Relay Module** - Điều khiển cửa/khóa điện
- **LED Status** - Báo hiệu trạng thái

### Sơ Đồ Kết Nối

| ESP32-C3 GPIO | Thiết Bị | Chức Năng |
|---------------|----------|-----------|
| GPIO 5 | LD2410 TX | UART TX |
| GPIO 6 | LD2410 RX | UART RX |
| GPIO 7 | Relay IN | Điều khiển relay |
| GPIO 8 | LED | Báo trạng thái phát hiện |

## ✨ Tính Năng

### 1. Phát Hiện Chuyển Động Thông Minh
- Sử dụng radar LD2410 phát hiện chuyển động và đối tượng tĩnh
- Đo khoảng cách chính xác (0-500cm)
- Đo năng lượng tín hiệu (Moving/Stationary Energy)

### 2. Điều Khiển Relay Tự Động
- **Chế độ AUTO**: Tự động bật relay khi phát hiện người ở khoảng cách đặt trước
- **Chế độ MANUAL**: Điều khiển relay hoàn toàn qua giao diện web
- **Anti-Flicker**: Relay bật tối thiểu 5 giây để tránh nhấp nháy
- Logic: HIGH = ON, LOW = OFF

### 3. Giao Diện Web

#### Kết Nối WiFi
- **SSID**: `DSS Door Opening Sensor`
- **Password**: `Dss@12345678`
- **IP Address**: `192.168.4.1`

#### Chức Năng Web Interface
- 📊 **Real-time Monitoring**: Hiển thị dữ liệu cảm biến theo thời gian thực (cập nhật 500ms)
  - Moving Distance & Energy
  - Stationary Distance & Energy
  - Trạng thái phát hiện người
  
- 🎚️ **Distance Threshold Slider**: Điều chỉnh ngưỡng khoảng cách (50-500cm)
  
- 🔄 **Auto/Manual Mode Switch**: Toggle giữa chế độ tự động và thủ công
  
- 🔘 **Manual Relay Control**: Nút bật/tắt relay thủ công (chỉ hoạt động ở chế độ MANUAL)

### 4. API Endpoints

| Endpoint | Method | Tham Số | Mô Tả |
|----------|--------|---------|-------|
| `/` | GET | - | Giao diện web chính |
| `/data` | GET | - | JSON data cảm biến |
| `/relay` | GET | - | Toggle relay thủ công |
| `/threshold` | GET | `value` (50-500) | Đặt ngưỡng khoảng cách |
| `/mode` | GET | `auto` (0/1) | Chuyển AUTO/MANUAL mode |

### 5. Debouncing Logic
```
Phát hiện người → Bật relay NGAY LẬP TỨC
Mất tín hiệu → Đợi MIN_ON_TIME (5s) → Tắt relay
```
Ngăn relay bật/tắt liên tục khi tín hiệu bị nhiễu.

## 📦 Cài Đặt

### Yêu Cầu
- [PlatformIO](https://platformio.org/) hoặc [PlatformIO IDE](https://platformio.org/install/ide?install=vscode)
- ESP32-C3 DevKit M-1
- Cáp USB-C

### Thư Viện Dependencies
```ini
ncmreynolds/ld2410@^0.1.3
bblanchon/ArduinoJson@^7.2.1
```

### Build & Upload
```bash
# Clone repository
git clone https://github.com/cuminhbecube/DSS-Door-Opening-Sensor.git
cd DSS-Door-Opening-Sensor

# Build project
pio run

# Upload to ESP32-C3
pio run --target upload --upload-port COMXX
```

## 🚀 Sử Dụng

1. **Khởi động thiết bị**: ESP32-C3 sẽ tạo WiFi AP tên `DSS Door Opening Sensor`
2. **Kết nối WiFi**: Dùng mật khẩu `Dss@12345678`
3. **Mở trình duyệt**: Truy cập `http://192.168.4.1`
4. **Cấu hình**:
   - Bật Auto Mode để relay tự động
   - Điều chỉnh Distance Threshold theo nhu cầu
   - Tắt Auto Mode để điều khiển thủ công

## 📝 Cấu Hình

### Thay Đổi Thời Gian Relay Bật Tối Thiểu
```cpp
const unsigned long MIN_ON_TIME = 5000; // 5 giây (ms)
```

### Thay Đổi WiFi Credentials
```cpp
const char* ssid = "DSS Door Opening Sensor";
const char* password = "Dss@12345678";
```

### Thay Đổi GPIO Pins
```cpp
const int RX_PIN = 6;       // LD2410 RX
const int TX_PIN = 5;       // LD2410 TX
const int STATUS_LED = 8;   // LED
const int RELAY_PIN = 7;    // Relay
```

## 🔍 Serial Monitor

Baudrate: `115200`

Output mẫu:
```
--- ESP32-C3 & LD2410 Door Control ---
LD2410 connected successfully!
WiFi AP started
SSID: DSS Door Opening Sensor
Password: Dss@12345678
IP Address: 192.168.4.1
Web server started!

Detected: MOVING 
  Moving -> Dist: 87cm, Energy: 45
Relay ON - Person detected
--------------------------------
Relay OFF - Minimum time elapsed
```

## 🛠️ Troubleshooting

### Cảm biến LD2410 không kết nối
- Kiểm tra kết nối UART (TX-RX, RX-TX)
- Đảm bảo nguồn 5V ổn định
- Baudrate: 256000

### Relay không hoạt động
- Kiểm tra GPIO 7 kết nối đúng
- Xác nhận logic relay (HIGH=ON, LOW=OFF)
- Kiểm tra nguồn relay module

### Web không truy cập được
- Kiểm tra đã kết nối đúng WiFi AP
- Thử reset ESP32-C3
- Ping `192.168.4.1`

## 📄 License

MIT License - Xem file [LICENSE](LICENSE)

## 👤 Tác Giả

**cuminhbecube**
- GitHub: [@cuminhbecube](https://github.com/cuminhbecube)
- Repository: [DSS-Door-Opening-Sensor](https://github.com/cuminhbecube/DSS-Door-Opening-Sensor)

## 🤝 Đóng Góp

Pull requests được chào đón! Đối với thay đổi lớn, vui lòng mở issue trước để thảo luận.

---

**Made with ❤️ for DSS Smart Door System**
