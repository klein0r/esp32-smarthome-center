/* @brief AUDIOKIT_BOARD selects a specic board:
 * 1) lyrat_v4_3
 * 2) lyrat_v4_2 - DRAFT Not Tested
 * 3) lyrat_mini_v1_1 - DRAFT Not Tested
 * 4) esp32_s2_kaluga_1_v1_2 - DRAFT Not Tested
 * 5) ai_thinker (ES8388) 2957 3478 A149
 * 6) ai_thinker (AC101) 2762 2957
 * 7) ai_thinker (ES8388) 2957
 * 10) generic_es8388
 */
#define AUDIOKIT_BOARD 5
#define SD_FAT_TYPE 2 // 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define AI_THINKER_ES8388_VOLUME_HACK 1
#define LED_GPIO 19

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define MQTT_SERVER "172.16.0.5"
#define MQTT_USER "esp"
#define MQTT_PASSWORD ""
#define MQTT_TOPIC "home/security/contact/reed/doorbell"

// ------------------------------------------------------

#define USE_SDFAT
#define USE_HELIX // or USE_MAD
#include "AudioTools.h"
#include "AudioLibs/AudioKit.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

const char *startFilePath = "/";
const char* ext = "mp3";
int speedMz = 10;

WiFiClient net;
PubSubClient client(net);

AudioSourceSdFat source(startFilePath, ext, PIN_AUDIO_KIT_SD_CARD_CS, speedMz);
AudioKitStream kit;
MP3DecoderHelix decoder; // or change to MP3DecoderMAD
AudioPlayer player(source, kit, decoder);

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == MQTT_TOPIC) {
    if (messageTemp == "on") {
      Serial.println("on");

      player.setIndex(1);
      player.begin();
    }
  }
}

void playDoorbellSound(bool, int, void*) {
    player.setIndex(1);
    player.begin();
}

void setup() {
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  setup_wifi();
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);

  // setup output
  auto cfg = kit.defaultConfig(TX_MODE);
  kit.begin(cfg);
  kit.setVolume(50);
  kit.addAction(PIN_KEY4, playDoorbellSound);

  // setup player
  player.setVolume(0.5);

  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, HIGH);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("smartcenter-esp", MQTT_USER, MQTT_PASSWORD)) {
      digitalWrite(LED_GPIO, LOW);

      Serial.println("connected");
      client.subscribe(MQTT_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    digitalWrite(LED_GPIO, HIGH);
    reconnect();
  }
  client.loop();

  player.copy();
  kit.processActions();
}
