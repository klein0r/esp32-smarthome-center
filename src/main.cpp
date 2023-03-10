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
#define MQTT_TOPIC_DOORBELL "esp32/sdsource"
#define MQTT_TOPIC_URLSOURCE "esp32/urlsource"
#define MQTT_TOPIC_VOLUME "esp32/volume"
#define MQTT_TOPIC_STOP "esp32/stop"

// ------------------------------------------------------

#define USE_SDFAT
#define USE_HELIX // #define USE_MAD
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h" // #include "AudioCodecs/CodecMP3MAD.h"
#include "AudioLibs/AudioKit.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

TaskHandle_t taskHandlePlayAudioURL = NULL;
TaskHandle_t taskHandlePlayAudioSD = NULL;

String urlToPlayFrom;
int sdCardIndexToPlay;

// WiFi + MQTT
WiFiClient net;
PubSubClient client(net);

AudioKitStream kit;
EncodedAudioStream dec(&kit, new MP3DecoderHelix()); // EncodedAudioStream dec(&kit, new MP3DecoderMAD());

// URL streaming
URLStream urlStream(WIFI_SSID, WIFI_PASSWORD);
StreamCopy copierUrl(dec, urlStream);

// SD card
const char *startFilePath = "/";
const char* ext = "mp3";
int speedMz = 10;

AudioSourceSdFat source(startFilePath, ext, PIN_AUDIO_KIT_SD_CARD_CS, speedMz);
StreamCopy copierSd;

void playUrl(void* url) {
  Serial.print("Playing-Task is running on Core: ");
  Serial.println(xPortGetCoreID());

  const char* urlToPlay = (*((String*)url)).c_str();

  // Play the audio from URL
  dec.begin();
  urlStream.begin(urlToPlay, "audio/mp3");
  copierUrl.copyAll(5, 1000);

  urlStream.end();
  dec.end();

  // Delete task
  Serial.println("Finished playing");
  Serial.print("Free Stack-Size for this task: ");
  Serial.println(uxTaskGetStackHighWaterMark(NULL));
  vTaskDelete(NULL);
}

void playSdIndex(void* index) {
  Serial.print("Playing-Task is running on Core: ");
  Serial.println(xPortGetCoreID());

  int indexToPlay = *((int*)index);

  // Play the audio from SD
  dec.begin();
  source.begin(); // Init SD

  Stream* inputStream = source.selectStream(indexToPlay);

  copierSd.begin(dec, *inputStream);
  copierSd.copyAll();

  dec.end();

  // Delete task
  Serial.println("Finished playing");
  Serial.print("Free Stack-Size for this task: ");
  Serial.println(uxTaskGetStackHighWaterMark(NULL));
  vTaskDelete(NULL);
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

  // Execute MQTT-command
  if (String(topic) == MQTT_TOPIC_DOORBELL) {
    if (messageTemp == "on") {
      Serial.println("Playing from SD");
      sdCardIndexToPlay = 0;
      xTaskCreate(playSdIndex, "Play sound from SD", 3072, (void*)&sdCardIndexToPlay, 1, &taskHandlePlayAudioSD);
    }
  }
  else if (String(topic) == MQTT_TOPIC_URLSOURCE) {
    Serial.println("Playing from URL");
    urlToPlayFrom = messageTemp;
    xTaskCreate(playUrl, "Play sound from URL", 3072, (void*)&urlToPlayFrom, 1, &taskHandlePlayAudioURL);
  }
  else if (String(topic) == MQTT_TOPIC_VOLUME) {
    Serial.println("Setting Volume");
    kit.setVolume(messageTemp.toInt());
  }
  else if (String(topic) == MQTT_TOPIC_STOP) {
    if(taskHandlePlayAudioURL != NULL) {
      Serial.println("Stopping playing -> reboot");
      vTaskDelete(taskHandlePlayAudioURL);
      urlStream.end();
      dec.end();
      ESP.restart();
    } else if (taskHandlePlayAudioSD != NULL) {
      Serial.println("Stopping playing -> reboot");
      vTaskDelete(taskHandlePlayAudioSD);
      urlStream.end();
      dec.end();
      ESP.restart();
    }
    else {
      Serial.println("Not playing");
    }
  }
}

void setup() {
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  AudioKitStreamConfig cfg = kit.defaultConfig(TX_MODE);
  cfg.sample_rate = 24000;
  cfg.channels = 1;
  // cfg.bits_per_sample = 16

  kit.begin(cfg);
  kit.setVolume(50);

  // Setup MQTT
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);

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
      client.subscribe(MQTT_TOPIC_DOORBELL);
      client.subscribe(MQTT_TOPIC_URLSOURCE);
      client.subscribe(MQTT_TOPIC_VOLUME);
      client.subscribe(MQTT_TOPIC_STOP);
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

  kit.processActions();
}
