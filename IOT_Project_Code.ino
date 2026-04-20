#define BLYNK_TEMPLATE_ID "TMPL6W1XSEZl2"
#define BLYNK_TEMPLATE_NAME "LASER SECURITY SYSTEM"
#define BLYNK_AUTH_TOKEN "zyZUjtqrqTDayq2DyCqSqfdAr8ZV73Ry"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <U8g2lib.h>

char ssid[] = "Priyam";
char pass[] = "Priyam@2407";

#define LASER_PIN     D5
#define LDR_PIN       D6
#define RED_LED       D3
#define GREEN_LED     D4
#define BUZZER_PIN    D7

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
BlynkTimer timer;

// ================= STATES =================
bool alarmActive = false;
bool systemArmed = true;
bool buzzerEnable = true;

// ================= VARIABLES =================
int alertCount = 0;
unsigned long lastBlink = 0;
bool ledState = false;

unsigned long lastBeep = 0;
bool buzzerState = false;

int stableCount = 0;
#define THRESHOLD_COUNT 5

int dotX = 0;
int scrollX = 128;

String lastStatus = "";
int lastAlertCount = -1;

// ✅ FIXED VARIABLES (ADDED)
unsigned long lastNotification = 0;
#define NOTIFY_INTERVAL 10000   // 10 sec cooldown
String location = "Main Gate";

// ================= BLYNK =================
BLYNK_WRITE(V2) { systemArmed = param.asInt(); }
BLYNK_WRITE(V5) { buzzerEnable = param.asInt(); }

// ================= SEND DATA =================
void sendBlynkData() {
  if (!systemArmed) {
    if (lastStatus != "OFF") {
      Blynk.virtualWrite(V0, "OFF ❌");
      Blynk.virtualWrite(V4, 0);
      Blynk.virtualWrite(V3, 0);
      lastStatus = "OFF";
    }
    return;
  }

  if (alarmActive) {
    Blynk.virtualWrite(V4, ledState ? 255 : 0);
    Blynk.virtualWrite(V3, 0);
    Blynk.virtualWrite(V0, "ALERT 🚨");

    if (lastAlertCount != alertCount) {
      Blynk.virtualWrite(V1, alertCount);
      lastAlertCount = alertCount;
    }
    lastStatus = "ALERT";
  } else {
    if (lastStatus != "SAFE") {
      Blynk.virtualWrite(V4, 0);
      Blynk.virtualWrite(V3, 255);
      Blynk.virtualWrite(V0, "SAFE ✅");
      lastStatus = "SAFE";
    }

    if (lastAlertCount != alertCount) {
      Blynk.virtualWrite(V1, alertCount);
      lastAlertCount = alertCount;
    }
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);

  pinMode(LASER_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LASER_PIN, HIGH);

  u8g2.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(500L, sendBlynkData);

  bootAnimation();
}

// ================= LOOP =================
void loop() {
  Blynk.run();
  timer.run();

  if (!systemArmed) {
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    showSystemOff();
    return;
  }

  int ldrValue = digitalRead(LDR_PIN);

  if (ldrValue == HIGH) stableCount++;
  else stableCount = 0;

  if (stableCount > THRESHOLD_COUNT) {

    if (!alarmActive) {
      alertCount++;
      alarmActive = true;

      if (millis() - lastNotification > NOTIFY_INTERVAL) {

        String msg = "👀 Someone just broke the laser!\n📍 Location: " + location +
                     "\nBro thinks he's invisible 💀🚨";

        Blynk.logEvent("intruder_alert", msg);

        lastNotification = millis();
      }
    }

    if (millis() - lastBlink > 200) {
      ledState = !ledState;
      digitalWrite(RED_LED, ledState);
      digitalWrite(GREEN_LED, LOW);
      lastBlink = millis();
    }

    if (buzzerEnable) {
      if (millis() - lastBeep > 200) {
        buzzerState = !buzzerState;
        digitalWrite(BUZZER_PIN, buzzerState);
        lastBeep = millis();
      }
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }

    alertAnimation();
  }

  else {
    alarmActive = false;
    ledState = false;

    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    safeScreen();
  }
}

// ================= BOOT =================
void bootAnimation() {
  for (int i = 0; i < 128; i += 4) {
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.drawBox(10, 30, i, 5);
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(25, 20, "Initializing...");
    u8g2.sendBuffer();
    delay(30);
  }
}

// ================= OFF =================
void showSystemOff() {
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(15, 15, "LASER SECURITY");
  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.drawStr(50, 38, "OFF");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(20, 55, "SYSTEM UNSAFE");
  u8g2.sendBuffer();
}

// ================= SAFE =================
void safeScreen() {
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);

  dotX += 2;
  if (dotX > 128) dotX = 0;
  u8g2.drawDisc(dotX, 28, 2);

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(15, 15, "LASER SECURITY");

  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.drawStr(38, 38, "SAFE");

  u8g2.setFont(u8g2_font_6x10_tr);
  String countStr = "Alerts: " + String(alertCount);
  u8g2.drawStr(25, 55, countStr.c_str());

  u8g2.sendBuffer();
}

// ================= ALERT =================
void alertAnimation() {
  u8g2.clearBuffer();

  if (ledState) u8g2.drawFrame(0, 0, 128, 64);

  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.drawStr(35, 30, "ALERT!");

  scrollX -= 3;
  if (scrollX < -100) scrollX = 128;

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(scrollX, 55, "INTRUDER DETECTED");

  u8g2.sendBuffer();
}