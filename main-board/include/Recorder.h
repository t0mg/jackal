#pragma once

#include <Arduino.h>
#include <SD.h>
#include <Audio.h>
#include <TimeLib.h>

class Recorder
{
public:
  Recorder(AudioPlaySdWav &playWav, AudioRecordQueue &queue);
  void setReverseAlphabeticalOrder(bool reverse) { reverseAlphabeticalOrder = reverse; }
  void startRecording();
  void continueRecording();
  void stopRecording();
  void play();
  void playFile(const char *filename);
  void playNextFile(bool wrap = false);
  void playPrevFile(bool wrap = false);
  void continuePlaying(bool andStop = false);
  void stopPlaying();
  bool deleteCurrentFile();
  bool seekToFirstFile();
  bool isRecording() const { return state == State::RECORDING; }
  bool isPlaying() const { return state == State::PLAYING; }
  const char *getCurrentFilename() const { return currentFilename; }
  void stopAll();

  static void parseTimestampFromFilename(const char *filename, char *dateStr, char *timeStr);
  void getMetadata(char *line1, char *line2);
  String getBasePath() { return basePath; }
  void setBasePath(String path) { basePath = path; currentFilename[0] = '\0'; }
  bool seek(bool previous = false, bool wrap = false);

private:
  bool reverseAlphabeticalOrder = true;
  String basePath;
  AudioPlaySdWav &playWav1;
  AudioRecordQueue &queue1;
  File frec;
  enum class State {
    STOPPED = 0,
    RECORDING = 1,
    PLAYING = 2
  };
  State state = State::STOPPED;
  elapsedMillis playStartDebounce = 0;
  char currentFilename[255] = "";
  char metadataLine1[30] = "";
  char metadataLine2[30] = "";
  bool temporaryMetadata = false;
  elapsedMillis metadataTimer = 0;
  unsigned long metadataTimeout = 5000;
  unsigned long lastSDOperation;
  static const unsigned long SD_COOLDOWN_MS = 500;
  unsigned long recordingStartTime;
  void writeWavHeader();
  void updateWavHeader(uint32_t fileSize);
  void playAdjacentFile(bool forward, bool wrap);
  static int compareFileNames(const void *a, const void *b);
  int listSortedFiles(char** fileList, int maxFiles);
};