#include <WiFi.h>
#include <HTTPClient.h>

// WiFi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server URLs
const char* alertURL = "http://your-server.com/esp32_alert";
const char* relayURL = "http://your-server.com/relay_state";

// Pin Definitions
#define SMPS1_FEEDBACK 6
#define SMPS2_FEEDBACK 7
#define SMPS1_CONTROL 25
#define SMPS2_CONTROL 23
#define BATTERY_CONTROL 24

int relayPins[] = {8, 9, 10, 11, 12, 13};

// State Flags
bool smps1_failed = false;
bool smps2_failed = false;

void setup() {
  Serial.begin(115200);

  // Configure I/O
  pinMode(SMPS1_FEEDBACK, INPUT);
  pinMode(SMPS2_FEEDBACK, INPUT);
  pinMode(SMPS1_CONTROL, OUTPUT);
  pinMode(SMPS2_CONTROL, OUTPUT);
  pinMode(BATTERY_CONTROL, OUTPUT);

  for (int i = 0; i < 6; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW); // All relays OFF
  }

  // Initial state
  digitalWrite(SMPS1_CONTROL, HIGH); // SMPS1 ON
  digitalWrite(SMPS2_CONTROL, LOW);  // SMPS2 OFF
  digitalWrite(BATTERY_CONTROL, LOW); // Battery OFF

  connectWiFi();
}

void loop() {
  managePower();
  checkRelayCommands();
  delay(5000); // Adjust as needed
}

// ==========================
// Power Management
// ==========================
void managePower() {
  // SMPS1 Check
  if (digitalRead(SMPS1_FEEDBACK) == LOW && !smps1_failed) {
    smps1_failed = true;
    sendAlert("SMPS1 failed. Switching to SMPS2.");
    digitalWrite(SMPS1_CONTROL, LOW); // Turn OFF SMPS1
    digitalWrite(SMPS2_CONTROL, HIGH); // Turn ON SMPS2
  }

  // SMPS2 Check
  if (digitalRead(SMPS2_FEEDBACK) == LOW && !smps2_failed) {
    smps2_failed = true;
    sendAlert("SMPS2 failed. Switching to battery.");
    digitalWrite(BATTERY_CONTROL, HIGH); // Turn ON Battery
  }
}

// ==========================
// Relay Control
// ==========================
void checkRelayCommands() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(relayURL);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Relay Command: " + payload);
    for (int i = 0; i < 6 && i < payload.length(); i++) {
      digitalWrite(relayPins[i], payload[i] == '1' ? HIGH : LOW);
    }
  } else {
    Serial.println("Failed to fetch relay state");
  }

  http.end();
}

// ==========================
// Server Alert
// ==========================
void sendAlert(String message) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(alertURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "message=" + message;
  int responseCode = http.POST(postData);

  if (responseCode > 0) {
    Serial.println("Alert sent: " + message);
  } else {
    Serial.println("Alert failed.");
  }

  http.end();
}

// ==========================
// WiFi Connection
// ===================

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());
}
