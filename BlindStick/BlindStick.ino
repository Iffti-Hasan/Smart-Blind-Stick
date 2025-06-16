#include <SoftwareSerial.h>
#include <TinyGPS++.h>

TinyGPSPlus gps;

// GPS on D11 (RX of Arduino), D10 (TX of Arduino)
SoftwareSerial gpsSerial(11, 10);

// SIM800L on D2 (RX of Arduino), D3 (TX of Arduino)
SoftwareSerial sim800l(2, 3);

// Emergency Button
#define BUTTON_PIN 6

// Ultrasonic Sensor
#define TRIG_PIN A1
#define ECHO_PIN A0

// Buzzer
#define BUZZER_PIN 5

// Phone number to send SMS
const char phoneNumber[] = "+8801703820793";

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("System ready. Waiting for GPS fix...");
}

void loop() {
  // Read GPS data
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Show GPS if available
  if (gps.location.isUpdated()) {
    Serial.print("Latitude: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(" | Longitude: ");
    Serial.println(gps.location.lng(), 6);
  }

  // Run obstacle check and control buzzer
  checkObstacle();

  // Check emergency button press
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed! Sending location...");
    delay(2000); // debounce and allow GPS to update

    if (gps.location.isValid()) {
      float lat = gps.location.lat();
      float lng = gps.location.lng();

      String message = "Emergency! Location: https://maps.google.com/?q=";
      message += String(lat, 6);
      message += ",";
      message += String(lng, 6);

      // Switch to SIM800L temporarily
      gpsSerial.end();
      sim800l.begin(9600);
      delay(1000);

      sendSMS(message);

      sim800l.end();
      gpsSerial.begin(9600);
    } else {
      Serial.println("GPS signal not ready. Try again.");
    }

    delay(5000); // avoid SMS spamming
  }
}

void checkObstacle() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2;

  if (distance > 0 && distance <= 60) {
    // ≤ 2 feet → continuous beep
    digitalWrite(BUZZER_PIN, HIGH);
  } 
  else if (distance > 60 && distance <= 90) {
    // 2–3 feet → intermittent beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  } 
  else {
    // > 3 feet → no beep
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void sendSMS(String msg) {
  sim800l.println("AT");
  delay(1000);

  sim800l.println("AT+CMGF=1"); // Text mode
  delay(1000);

  sim800l.print("AT+CMGS=\"");
  sim800l.print(phoneNumber);
  sim800l.println("\"");
  delay(1000);

  sim800l.print(msg);
  delay(500);
  sim800l.write(26); // CTRL+Z
  delay(5000);

  Serial.println("SMS Sent: " + msg);
}
