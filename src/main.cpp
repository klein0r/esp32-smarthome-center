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

const char *startFilePath = "/";
const char* ext = "mp3";
int speedMz = 10;
AudioSourceSdFat source(startFilePath, ext, PIN_AUDIO_KIT_SD_CARD_CS, speedMz);
AudioKitStream kit;
MP3DecoderHelix decoder; // or change to MP3DecoderMAD
AudioPlayer player(source, kit, decoder);

void next(bool, int, void*) {
   player.next();
}

void previous(bool, int, void*) {
   player.previous();
}

void setup() {
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Info);

  // setup output
  auto cfg = kit.defaultConfig(TX_MODE);
  kit.begin(cfg);

  // setup additional buttons 
  kit.setVolume(1.0);
  kit.addAction(PIN_KEY4, next);
  kit.addAction(PIN_KEY3, previous);

  // setup player
  player.setVolume(1.0);
  player.begin();
}

void loop() {
  player.copy();
  kit.processActions();
}
