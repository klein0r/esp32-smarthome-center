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

#define USE_SDFAT
#define USE_HELIX // or USE_MAD
#include "AudioTools.h"
#include "AudioLibs/AudioKit.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

// Replace the next variables with your SSID/Password combination
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "172.16.0.5";
const char* mqtt_user =  "";
const char* mqtt_password = "";

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
  Serial.println(ssid);

  WiFi.begin(ssid, password);

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

  if (String(topic) == "esp32/output") {
    if (messageTemp == "on") {
      Serial.println("on");

      player.setIndex(1);
      player.begin();
    }
  }
}

void setup() {
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // setup output
  auto cfg = kit.defaultConfig(TX_MODE);
  kit.begin(cfg);
  kit.setVolume(50);

  // setup player
  player.setVolume(0.5);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("smartcenter-esp", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("esp32/output");
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
    reconnect();
  }
  client.loop();

  if (player.isActive()) {
    player.copy();
  }
}
