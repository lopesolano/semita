/*
  Control de giro con sensor - EAI441 (Opcion 2: Web y API separadas)
  Placa: Arduino UNO R4 WiFi + Motor Shield v1.2 (L293D + 74HC595, DK Electronics)
  Librerias: ArduinoHttpClient, ArduinoJson (NO usar AFMotor: no compila en la R4)

  Conexiones:
    Motor DC (TT amarillo) -> borne M3 del shield (abajo a la izquierda)
    Alimentacion de motores -> borne EXT_PWR (+M / GND), 4.5 a 6 V
    HC-SR04: VCC -> 5V, GND -> GND, TRIG -> A0, ECHO -> A1
*/
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>   // instalar "ArduinoJson" (Benoit Blanchon)

// ---------- CONFIGURACION ----------
const char* WIFI_SSID = "TIGO-4290";
const char* WIFI_PASS = "704FB8294290";
const char* API_HOST  = "semita-api.onrender.com";   // sin https://
const int   API_PORT  = 443;                     // 443 = HTTPS
const char* API_KEY   = "DEVICE_KEY";     // igual que DEVICE_KEY en la API
const int   MOTOR_SPEED = 200;                   // 0-255
// -----------------------------------

// Sensor
const int TRIG = A0, ECHO = A1;

// Shield de motores v1 (registro de desplazamiento 74HC595)
const int MOTORLATCH = 12, MOTORCLK = 4, MOTORENABLE = 7, MOTORDATA = 8;
const int M3_PWM = 6;                 // PWM del motor M3
const int M3_A_BIT = 5, M3_B_BIT = 7; // bits del 74HC595 para M3

void shiftWrite(uint8_t value) {
  digitalWrite(MOTORLATCH, LOW);
  shiftOut(MOTORDATA, MOTORCLK, MSBFIRST, value);
  digitalWrite(MOTORLATCH, HIGH);
}

WiFiSSLClient wifi;                              // para HTTP plano usar WiFiClient y puerto 80
HttpClient http(wifi, API_HOST, API_PORT);

String motorState = "stop";
unsigned long lastSync = 0, lastOk = 0;
const unsigned long SYNC_MS = 500;
const unsigned long FAILSAFE_MS = 3000;          // si no hay respuesta de la API, detener motor

void setMotor(const String& cmd) {
  uint8_t bits = 0;
  if (cmd == "right") {            // si gira al reves, intercambia los dos cables en M3
    bits = (1 << M3_A_BIT); analogWrite(M3_PWM, MOTOR_SPEED);
  } else if (cmd == "left") {
    bits = (1 << M3_B_BIT); analogWrite(M3_PWM, MOTOR_SPEED);
  } else {
    analogWrite(M3_PWM, 0); motorState = "stop"; shiftWrite(0); return;
  }
  shiftWrite(bits);
  motorState = cmd;
}

float readDistanceCm() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long us = pulseIn(ECHO, HIGH, 30000);          // timeout 30 ms (~5 m)
  if (us == 0) return -1;                        // sin eco
  return us * 0.0343 / 2.0;
}

void connectWifi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Conectando a "); Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(5000);
  }
  Serial.print("WiFi OK, IP: "); Serial.println(WiFi.localIP());
}

void syncWithApi() {
  float d = readDistanceCm();

  JsonDocument body;
  body["distance"] = d < 0 ? -1 : round(d * 10) / 10.0;
  body["motor"] = motorState;
  String payload; serializeJson(body, payload);

  http.beginRequest();
  http.post("/api/device/sync");
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", payload.length());
  http.sendHeader("x-api-key", API_KEY);
  http.beginBody();
  http.print(payload);
  http.endRequest();

  int status = http.responseStatusCode();
  String resp = http.responseBody();
  if (status == 200) {
    JsonDocument doc;
    if (!deserializeJson(doc, resp)) {
      setMotor(doc["command"] | "stop");
      lastOk = millis();
    }
  } else {
    Serial.print("Error API: "); Serial.println(status);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(MOTORLATCH, OUTPUT); pinMode(MOTORCLK, OUTPUT);
  pinMode(MOTORENABLE, OUTPUT); pinMode(MOTORDATA, OUTPUT); pinMode(M3_PWM, OUTPUT);
  digitalWrite(MOTORENABLE, LOW);   // habilita las salidas del 74HC595 (activo en bajo)
  setMotor("stop");
  connectWifi();
  lastOk = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { setMotor("stop"); connectWifi(); }

  if (millis() - lastSync >= SYNC_MS) {
    lastSync = millis();
    syncWithApi();
  }
  if (millis() - lastOk > FAILSAFE_MS) setMotor("stop");   // failsafe
}
