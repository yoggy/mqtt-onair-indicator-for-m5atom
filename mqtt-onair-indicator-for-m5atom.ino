#include "M5Atom.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#include "config.h"

WiFiClient wifi_client;
void mqtt_sub_callback(char* topic, byte* payload, unsigned int length);
PubSubClient mqtt_client(mqtt_host, mqtt_port, mqtt_sub_callback, wifi_client);

unsigned long last_t = 0;
bool is_on_air = false;

void setup() {
  Serial.begin(115200);
  M5.begin(true, false, true);
  M5.dis.drawpix(0, 0xffffff);  // white

  // Wifi
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    switch (count) {
      case 0:
        Serial.println("|");
        M5.dis.drawpix(0, 0xffff00);  // yellow
        break;
      case 1:
        Serial.println("/");
        break;
      case 2:
        M5.dis.drawpix(0, 0x000000);  // black
        Serial.println("-");
        break;
      case 3:
        Serial.println("\\");
        break;
    }
    count = (count + 1) % 4;
  }
  WiFi.setSleep(false);
  Serial.println("WiFi connected!");
  delay(1000);

  // MQTT
  bool rv = false;
  if (mqtt_use_auth == true) {
    rv = mqtt_client.connect(mqtt_client_id, mqtt_username, mqtt_password);
  }
  else {
    rv = mqtt_client.connect(mqtt_client_id);
  }
  if (rv == false) {
    Serial.println("mqtt connecting failed...");
    reboot();
  }
  Serial.println("MQTT connected!");
  delay(1000);

  M5.dis.drawpix(0, 0x000088);  // blue

  mqtt_client.subscribe(mqtt_onair_data_topic);

  last_t = millis();

  // configTime
  configTime(9 * 3600, 0, "ntp.nict.jp");
  struct tm t;
  if (!getLocalTime(&t)) {
    Serial.println("getLocalTime() failed...");
    delay(1000);
    reboot();
  }
  Serial.println("configTime() success!");
  M5.dis.drawpix(0, 0x008800);  // green

  delay(1000);
}

void reboot() {
  Serial.println("REBOOT!!!!!");
  for (int i = 0; i < 30; ++i) {
    fillPixel(0xff00ff); // magenta
    delay(100);
    fillPixel(0x000000);
    delay(100);
  }

  ESP.restart();
}

void loop() {
  mqtt_client.loop();
  if (!mqtt_client.connected()) {
    Serial.println("MQTT disconnected...");
    reboot();
  }

  M5.update();

  if (M5.Btn.wasPressed()) {
    is_on_air = !is_on_air;
    if (is_on_air) {
      Serial.println("publish on_air");
      mqtt_client.publish(mqtt_onair_data_topic, "on_air", true);
    }
    else {
      Serial.println("publish off_air");
      mqtt_client.publish(mqtt_onair_data_topic, "off_air", true);
    }
  }
}

#define BUF_LEN 16
char buf[BUF_LEN];

void mqtt_sub_callback(char* topic, byte* payload, unsigned int length) {

  int len = BUF_LEN - 1 < length ? (BUF_LEN - 1) : length;
  memset(buf, 0, BUF_LEN);
  strncpy(buf, (const char*)payload, len);

  String cmd = String(buf);
  Serial.print("payload=");
  Serial.println(cmd);

  if (cmd == "on_air") {
    Serial.println("set on_air status");
    fillPixel(0x880000); // red
    is_on_air = true;
  }
  else if (cmd == "off_air") {
    Serial.println("set off_air status");
    onePixel(0x008800); // green
    is_on_air = false;
  }
}

void fillPixel(uint32_t color)
{
  for (uint32_t i = 0; i < 25; i++) {
    M5.dis.drawpix(i, color);  // red
    is_on_air = true;
  }
}

void onePixel(uint32_t color)
{
  M5.dis.drawpix(0, color);
  for (uint32_t i = 1; i < 25; i++) {
    M5.dis.drawpix(i, 0);  // black
    is_on_air = true;
  }
}
