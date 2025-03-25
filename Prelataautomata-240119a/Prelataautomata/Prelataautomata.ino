#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "Your SSID";
const char* password = "Your password";

const int hallEffectPin = A0;
const int motorPin = D1;  

const float defaultWindSpeedThreshold = 40.0;
float windSpeedThreshold = defaultWindSpeedThreshold;

const unsigned long sampleInterval = 1000;
unsigned long lastSampleTime = 0;
unsigned long pulseCount = 0;

AsyncWebServer server(80);

void IRAM_ATTR handlePulse() {
  pulseCount++;
}

float calculateWindSpeed() {
  unsigned long elapsedTime = millis() - lastSampleTime;
  float windSpeed = (float)pulseCount / (elapsedTime / 1000.0);
  return windSpeed;
}

String getHtmlContent(float windSpeed) {
  String html = "<html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Wind Speed Monitor</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; text-align: center; background-color: #f4f4f4; color: #333; }";
  html += ".container { max-width: 800px; margin: auto; padding: 20px; }";
  html += "h1 { color: #007BFF; }";
  html += "#windThreshold { width: 100%; margin-top: 10px; }";
  html += "#thresholdValue { margin-top: 10px; }";
  html += ".button { background-color: #007BFF; color: white; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; }";
  html += "@media only screen and (min-width: 600px) { .container { max-width: 600px; } }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>Wind Speed Monitor</h1>";
  html += "<p>Current Wind Speed: <span id='currentWindSpeed'>" + String(windSpeed, 2) + "</span> km/h</p>";
  html += "<label for='windThreshold'>Wind Speed Tolerance:</label>";
  html += "<input type='range' id='windThreshold' name='windThreshold' min='30' max='70' value='" + String(windSpeedThreshold) + "' oninput='updateThreshold(value)'>";
  html += "<p id='thresholdValue'>Tolerance: " + String(windSpeedThreshold) + " km/h</p>";
  html += "<button class='button' onclick='toggleMotor()'>Toggle Motor</button>";
  html += "<p>IP Address: " + WiFi.localIP().toString() + "</p>";
  html += "</div>";
  html += "<script>";
  html += "function updateThreshold(value) { document.getElementById('thresholdValue').innerHTML = 'Tolerance: ' + value + ' km/h'; }";
  html += "function toggleMotor() { fetch('/toggleMotor'); }";
  html += "</script></body></html>";
  return html;
}

void setup() {
  Serial.begin(115200);

  pinMode(hallEffectPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(hallEffectPin), handlePulse, RISING);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    float windSpeed = calculateWindSpeed();
    String content = getHtmlContent(windSpeed);
    request->send(200, "text/html", content);
  });
  server.on("/toggleMotor", HTTP_GET, [](AsyncWebServerRequest* request) {
    // Toggle the motor
    digitalWrite(motorPin, !digitalRead(motorPin));
    request->send(200, "text/plain", "Motor toggled");
  });

  server.begin();
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastSampleTime >= sampleInterval) {
    lastSampleTime = currentTime;
    pulseCount = 0;
  }

  float windSpeed = calculateWindSpeed();
  Serial.print("Wind Speed: ");
  Serial.print(windSpeed);
  Serial.println(" km/h");

  if (windSpeed > windSpeedThreshold) {
    digitalWrite(motorPin, HIGH);
    Serial.println("Motor Activated!");
  } else {
    digitalWrite(motorPin, LOW);
    Serial.println("Motor Deactivated.");
  }

  delay(5000);
}
