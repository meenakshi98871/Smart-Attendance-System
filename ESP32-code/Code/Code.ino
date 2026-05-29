#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "WIFI NAME";
const char* password = "WIFI PASSWORD";

// Google Apps Script Web App URL
String serverName = APPSCRIPT_URL;

// OLED configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// RTC
RTC_DS1307 rtc;

// Button & LED Pins
#define BUTTON_FINGERPRINT 26  // Green button
#define BUTTON_RFID 27         // Blue button
#define LED_GREEN 14
#define LED_RED 12

bool lastFingerprintState = HIGH;
bool lastRFIDState = HIGH;

bool fingerprintVerified = false;
bool rfidVerified = false;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_FINGERPRINT, INPUT_PULLUP);
  pinMode(BUTTON_RFID, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  Wire.begin();

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED not found");
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // RTC
  if (!rtc.begin()) {
    Serial.println("❌ RTC not found");
    while (true);
  }

  if (!rtc.isrunning()) {
    Serial.println("⏰ RTC not running. Setting time...");
    rtc.adjust(DateTime(2025, 7, 16, 10, 30, 0)); // Set current date/time
  }

  // WiFi
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("✅ WiFi Connected");
  display.println("Place Finger + RFID");
  display.display();
}

void loop() {
  bool currentFingerprintState = digitalRead(BUTTON_FINGERPRINT);
  bool currentRFIDState = digitalRead(BUTTON_RFID);

  // Fingerprint check
  if (currentFingerprintState == LOW && lastFingerprintState == HIGH) {
    fingerprintVerified = true;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("✅ Fingerprint OK");
    display.display();
    Serial.println("Fingerprint verified");
    delay(300);
  }

  // RFID check
  if (currentRFIDState == LOW && lastRFIDState == HIGH) {
    rfidVerified = true;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("✅ RFID OK");
    display.display();
    Serial.println("RFID verified");
    delay(300);
  }

  // If both are verified
  if (fingerprintVerified && rfidVerified) {
    handleAttendance("Meenakshi");  // Use your desired name or ID
    fingerprintVerified = false;
    rfidVerified = false;
  }

  lastFingerprintState = currentFingerprintState;
  lastRFIDState = currentRFIDState;
}

void handleAttendance(String name) {
  DateTime now = rtc.now();
  String hourStr = (now.hour() < 10 ? "0" : "") + String(now.hour());
  String minStr = (now.minute() < 10 ? "0" : "") + String(now.minute());
  String timeStr = hourStr + ":" + minStr;
  String status = "YES";

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Sending to Sheets...");
  display.println("Name: " + name);
  display.println("Time: " + timeStr);
  display.display();

  Serial.println("Name: " + name);
  Serial.println("Time: " + timeStr);
  Serial.println("Status: " + status);

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 5000) {
      delay(500);
      Serial.print(".");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    String fullURL = serverName + "?Name=" + name + "&Time=" + timeStr + "&Status=" + status;
    http.begin(fullURL);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("✅ Sent: " + response);
    } else {
      Serial.println("❌ Error Code: " + String(httpCode));
      digitalWrite(LED_RED, HIGH);
      delay(300);
      digitalWrite(LED_RED, LOW);
    }

    http.end();
  } else {
    Serial.println("❌ WiFi not connected.");
  }

  digitalWrite(LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("✅ Attendance Done");
  display.println("Press Finger + RFID");
  display.display();
}