#include "PlaceOccupancyStateMachine.hpp"
#include "secrets.hpp"

#include <HTTPClient.h>
#include <WiFi.h>
#include <vector>

// TODO: dynamic url, update it before running
const char *SERVER_URL = "https://4jel2guyd.localto.net";

// Hardware config
const uint8_t sonarPin = 14;
const uint8_t servoPin = 12;
std::vector<uint8_t> ledPins = {21, 13, 27, 4};

// SM config
constexpr uint8_t TOTAL_SEATS = 4;
constexpr float ANGLE_PER_SEAT = 20.0f;
constexpr uint32_t SCAN_INTERVAL_S = 10;

constexpr uint32_t POST_INTERVAL_MS = 10000;

OccupancyStateMachine occupancySM{ledPins, servoPin, sonarPin,
                                  ANGLE_PER_SEAT, SCAN_INTERVAL_S, TOTAL_SEATS};

void connectToWiFi()
{
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFiClass::mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retryCount = 0;
  while (WiFiClass::status() != WL_CONNECTED && retryCount < 30)
  {
    delay(500);
    Serial.print(".");
    retryCount++;
  }

  if (WiFiClass::status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

void sendOccupancy(int freeSeats, int totalSeats)
{
  if (WiFiClass::status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected, skipping POST");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String postUrl = String(SERVER_URL) + "/api/occupancy";
  Serial.println(String("Posting to: ") + postUrl);

  if (!http.begin(client, postUrl))
  {
    Serial.println("http.begin() failed");
    return;
  }
  http.setReuse(false);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("localtonet-skip-warning", "1");

  String payload = "{";
  payload += "\"node_id\":\"lb8-node-1\",";
  payload += "\"free_seats\":" + String(freeSeats) + ",";
  payload += "\"total_seats\":" + String(totalSeats);
  payload += "}";

  Serial.println("POST to /api/occupancy");
  Serial.println(payload);

  int httpCode = http.POST(payload);

  if (httpCode > 0)
  {
    Serial.printf("HTTP %d\n", httpCode);
    Serial.println("Response:");
    Serial.println(http.getString());
  }
  else
  {
    Serial.printf("POST failed: %d (%s)\n", httpCode,
                  HTTPClient::errorToString(httpCode).c_str());
  }

  http.end();
}

void setup()
{
  Serial.begin(115200);
  delay(2000);

  connectToWiFi();
  occupancySM.begin();
}

void loop()
{
  static unsigned long lastPostMs = 0;
  unsigned long now = millis();

  occupancySM.update();

  // Periodically send occupancy data
  if (now - lastPostMs >= POST_INTERVAL_MS)
  {
    lastPostMs = now;

    // TODO: light up bulb based on seats.
    int freeSeats = occupancySM.emptySeats();
    int totalSeats = TOTAL_SEATS;

    Serial.print("Reporting occupancy: free=");
    Serial.print(freeSeats);
    Serial.print(" / total=");
    Serial.println(totalSeats);

    sendOccupancy(freeSeats, totalSeats);
  }

  delay(10);
}
