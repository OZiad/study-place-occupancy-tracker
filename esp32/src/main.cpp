#include "secrets.hpp"
#include <HTTPClient.h>
#include <WiFi.h>

// TODO OMAR: replace with laptop's IP address (use `ip addr`)
const char *SERVER_URL = "http://192.168.0.11:8000/api/occupancy";

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFiClass::mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retryCount = 0;
  while (WiFiClass::status() != WL_CONNECTED && retryCount < 30) {
    delay(500);
    Serial.print(".");
    retryCount++;
  }

  if (WiFiClass::status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

void sendOccupancy(int freeSeats, int totalSeats) {
  if (WiFiClass::status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping POST");
    return;
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += R"("node_id":"ev8-node-1",)";
  payload += "\"free_seats\":" + String(freeSeats) + ",";
  payload += "\"total_seats\":" + String(totalSeats);
  payload += "}";

  Serial.println("POSTing to server: ");
  Serial.println(payload);

  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    String resp = http.getString();
    Serial.println("Response:");
    Serial.println(resp);
  } else {
    Serial.printf("POST failed, error: %s\n",
                  http.errorToString(httpCode).c_str());
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  connectToWiFi();
}

void loop() {
  // dummy data for now
  int totalSeats = 4;
  static int fakeFree = 0;
  fakeFree = (fakeFree + 1) % (totalSeats + 1); // cycles 0..4

  sendOccupancy(fakeFree, totalSeats);

  // send every 10 seconds
  delay(10000);
}
