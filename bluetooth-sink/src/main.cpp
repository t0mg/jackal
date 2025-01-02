#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "BluetoothA2DPSink.h"
#include "AudioTools.h"

#define DEBUG_BT_AUDIO false

#define I2C_ADDRESS 0x03
#define MAX_BT_DATA_LENGTH 136

#define CONFIG_I2S_LRCK_PIN G22
#define CONFIG_I2S_BCK_PIN G19
#define CONFIG_I2S_DATA_PIN G23

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, G27, NEO_GRB + NEO_KHZ800);
unsigned long last = 0;

// Add these global variables to store metadata
struct MetadataStore
{
  String title;
  String artist;
  unsigned long lastUpdate;
  esp_avrc_playback_stat_t playbackState;
  bool isConnected;
} metadata;

volatile byte i2cRegister = 0xff;

// Add to global variables
#define COMMAND_COOLDOWN 1000 // 1 second cooldown between commands
volatile bool needsRecovery = false;
volatile uint8_t lastAudioState = 2;

// Debug logging macros
#if DEBUG_BT_AUDIO
#define LOG_DEBUG(x) Serial.println(x)
#define LOG_DEBUGF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
#define LOG_DEBUG(x)
#define LOG_DEBUGF(x, ...)
#endif

// Add to global variables
#define ESP_AVRC_RN_PLAY_POS_CHANGED 0x05

void i2cReceive(int numBytes)
{
  static unsigned long lastCommandTime = 0;
  bool validCommand = false;

  LOG_DEBUGF("I2C Receive called with %d bytes\n", numBytes);
  pixels.setPixelColor(0, pixels.Color(0, 0, 255));

  if (numBytes < 1)
  {
    LOG_DEBUG("No bytes received");
    goto cleanup;
  }

  i2cRegister = Wire.read();
  LOG_DEBUGF("Command received: '%c' (0x%02X)\n", i2cRegister, i2cRegister);

  // Check cooldown
  if (millis() - lastCommandTime < COMMAND_COOLDOWN)
  {
    LOG_DEBUG("Command ignored - in cooldown period");
    pixels.setPixelColor(0, pixels.Color(255, 165, 0));
    goto cleanup;
  }

  // Process command
  switch (i2cRegister)
  {
  case 'p': // play
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    a2dp_sink.play();
    validCommand = true;
    break;
  case 's': // stop
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    a2dp_sink.pause();
    validCommand = true;
    break;
  case 'r': // previous
    a2dp_sink.previous();
    validCommand = true;
    break;
  case 'n': // next
    a2dp_sink.next();
    validCommand = true;
    break;
  }

  if (validCommand)
  {
    lastCommandTime = millis();
    LOG_DEBUGF("Executed command: %c\n", i2cRegister);
  }

cleanup:
  // Clean up I2C buffer
  while (Wire.available())
  {
    Wire.read();
  }
  pixels.show();
}

void i2cRequest()
{
  static unsigned long lastRequestTime = 0;
  static uint16_t requestCount = 0;
  const int MAX_FIELD_LENGTH = 32;
  static char buffer[MAX_BT_DATA_LENGTH];

  unsigned long now = millis();
  unsigned long delta = now - lastRequestTime;
  LOG_DEBUGF("I2C Request at %lu (delta: %lu ms)\n", now, delta);

  if (delta > 1000)
  {
    LOG_DEBUG("Large timing gap detected!");
    LOG_DEBUGF("Audio State: %d\n", a2dp_sink.get_audio_state());
    LOG_DEBUGF("I2S Sample Rate: %d\n", a2dp_sink.sample_rate());
    LOG_DEBUGF("Wire Available: %d\n", Wire.available());
  }
  LOG_DEBUGF("Attempting to write %d bytes\n", MAX_BT_DATA_LENGTH);

  // Build response string
  String dataToSend = "";

  if (!a2dp_sink.is_connected())
  {
    dataToSend += "|T-|A-|SUNKNOWN|Cdisconnected";
  }
  else
  {
    const char *stateStr = (metadata.playbackState == ESP_AVRC_PLAYBACK_PLAYING) ? "PLAYING" : "STOPPED";

    if (metadata.playbackState == ESP_AVRC_PLAYBACK_PLAYING)
    {
      dataToSend += "|T" + (metadata.title.length() ? metadata.title.substring(0, MAX_FIELD_LENGTH) : "-");
      dataToSend += "|A" + (metadata.artist.length() ? metadata.artist.substring(0, MAX_FIELD_LENGTH) : "-");
    }
    else
    {
      dataToSend += "|T-|A-";
    }

    dataToSend += "|S" + String(stateStr);
    dataToSend += "|C" + String(a2dp_sink.get_peer_name()).substring(0, MAX_FIELD_LENGTH);
  }

  // Copy to buffer with null termination
  strncpy(buffer, dataToSend.c_str(), MAX_BT_DATA_LENGTH - 1);
  buffer[MAX_BT_DATA_LENGTH - 1] = '\0';

  Wire.write((uint8_t *)buffer, MAX_BT_DATA_LENGTH);

  LOG_DEBUGF("Write complete. Sent: %s\n", buffer);
  lastRequestTime = now;
}

void avrc_metadata_callback(uint8_t id, const uint8_t *text)
{
  switch (id)
  {
  case ESP_AVRC_MD_ATTR_TITLE:
    metadata.title = String((char *)text);
    break;
  case ESP_AVRC_MD_ATTR_ARTIST:
    metadata.artist = String((char *)text);
    break;
  }
  metadata.lastUpdate = millis();
}

void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback)
{
  metadata.playbackState = playback;
  metadata.lastUpdate = millis();

  if (playback == ESP_AVRC_PLAYBACK_STOPPED ||
      playback == ESP_AVRC_PLAYBACK_PAUSED)
  {
    LOG_DEBUG("Audio playback stopped");
    LOG_DEBUGF("A2DP Audio State: %d\n", a2dp_sink.get_audio_state());
    LOG_DEBUGF("A2DP Audio Type: %d\n", a2dp_sink.get_audio_type());
    LOG_DEBUGF("I2S Sample Rate: %d\n", a2dp_sink.sample_rate());

    // Set up timer to monitor for BT stack state changes
    static hw_timer_t *timer = NULL;
    if (timer == NULL)
    {
      timer = timerBegin(0, 80, true); // 1MHz timer
      timerAttachInterrupt(timer, []()
                           {
                uint8_t currentState = a2dp_sink.get_audio_state();
                
                // Detect transition from active (2) to idle (0)
                if (lastAudioState == 2 && currentState == 0) {
                    needsRecovery = true;
                }
                lastAudioState = currentState; }, true);
      timerAlarmWrite(timer, 600000, true); // Check every 600 ms
      timerAlarmEnable(timer);
    }
  }
  else if (playback == ESP_AVRC_PLAYBACK_PLAYING)
  {
    LOG_DEBUG("Audio playback started");
    LOG_DEBUGF("A2DP Audio State: %d\n", a2dp_sink.get_audio_state());
    LOG_DEBUGF("A2DP Audio Type: %d\n", a2dp_sink.get_audio_type());
    LOG_DEBUGF("I2S Sample Rate: %d\n", a2dp_sink.sample_rate());
  }
}

void connection_state_changed(esp_a2d_connection_state_t state, void *ptr)
{
  metadata.isConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
  metadata.lastUpdate = millis();

  // Clear all metadata on disconnect
  if (!metadata.isConnected)
  {
    metadata.title = "";
    metadata.artist = "";
    metadata.playbackState = ESP_AVRC_PLAYBACK_STOPPED;
  }
}

void setup()
{
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = CONFIG_I2S_BCK_PIN;
  cfg.pin_ws = CONFIG_I2S_LRCK_PIN;
  cfg.pin_data = CONFIG_I2S_DATA_PIN;
  cfg.pin_data_rx = -1; // Not used
  cfg.bits_per_sample = (i2s_bits_per_sample_t)16;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.rx_tx_mode = TX_MODE;
  cfg.is_master = true;
  cfg.i2s_format = I2S_STD_FORMAT;
  cfg.use_apll = true;
  cfg.auto_clear = true;
  cfg.buffer_count = 8;
  cfg.buffer_size = 128;
  cfg.sample_rate = 44100;
  i2s.begin(cfg);

  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();

  Wire.setPins(G25, G21);
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(i2cReceive);
  Wire.onRequest(i2cRequest);

#if DEBUG_BT_AUDIO
  Serial.begin(115200);
  Serial.println("Jackal bt");
#endif

  pixels.setPixelColor(0, pixels.Color(255, 255, 0));
  pixels.show();
  last = millis();

  a2dp_sink.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  // a2dp_sink.set_auto_reconnect(true);

  // Register the playback state callback
  a2dp_sink.set_avrc_rn_playstatus_callback(avrc_rn_playstatus_callback);

  // Initialize playback state
  metadata.playbackState = ESP_AVRC_PLAYBACK_STOPPED;

  // Initialize connection state
  metadata.isConnected = false;

  // Register the connection state callback
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);

  a2dp_sink.start("Jackal");
}

void loop()
{
  // Perform recovery if BT stack state change was detected
  if (needsRecovery)
  {
    LOG_DEBUG("Attempting recovery");
    i2s.begin();
    esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
    needsRecovery = false;
  }

  // LED handling
  if (last != 0 && (millis() - last) > 1000)
  {
    last = millis();
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
  }
  delay(10);
}
