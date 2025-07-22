#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// Firebase Realtime Database URL
const char* firebaseHost = "https://pawme-81fbe-default-rtdb.firebaseio.com";

// WiFi credentials
const char* ssid = "GalaxyM02s2924";
const char* password = "yrok5164";

// GPS and sensor setup
#define rx 2
#define tx 1
#define sda 9
#define scl 10

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

// Timezone offset (GMT+5:30 = 19800 seconds)
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  // Set up NTP for time
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  // Start sensors
  Wire.begin(sda, scl);
  if (!mlx.begin()) {
    Serial.println("MLX90614 not found!");
    while (1);
  }

  gpsSerial.begin(9600, SERIAL_8N1, rx, tx);
  Serial.println("Setup complete. Reading data...");
}

void loop() {
  // GPS data handling
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Send temperature to Firebase
  float tempC = mlx.readObjectTempC();
  float ambTempC = mlx.readAmbientTempC();
  String timestamp = getFormattedTime();

  sendTemperatureToFirebase(tempC, timestamp);

  // Send GPS if location updated
  if (gps.location.isUpdated()) {
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();
    sendLocationToFirebase(latitude, longitude);
  }

  // Debug output
  Serial.println("Data sent to Firebase:");
  Serial.print("Temp: "); Serial.print(tempC); Serial.println(" °C");
  Serial.print("Timestamp: "); Serial.println(timestamp);
  if (gps.location.isUpdated()) {
    Serial.print("Lat: "); Serial.println(gps.location.lat(), 6);
    Serial.print("Lng: "); Serial.println(gps.location.lng(), 6);
  }

  Serial.println("----------------------------");
  delay(5000); // Adjust as needed
}

// ---- Helper Functions ----

void sendTemperatureToFirebase(float temp, String timestamp) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(firebaseHost) + "/temperatures.json";

    // Construct JSON
    String jsonData = "{\"temperature\":" + String(temp, 2) + ",\"timestamp\":\"" + timestamp + "\"}";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.PUT(jsonData);

    Serial.print("Temp PUT Response: ");
    Serial.println(httpResponseCode);
    http.end();
  }
}

void sendLocationToFirebase(float lat, float lng) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(firebaseHost) + "/location.json";

    // Construct JSON
    String jsonData = "{\"latitude\":" + String(lat, 6) + ",\"longitude\":" + String(lng, 6) + "}";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.PUT(jsonData);

    Serial.print("Location PUT Response: ");
    Serial.println(httpResponseCode);
    http.end();
  }
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    return "N/A";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}
