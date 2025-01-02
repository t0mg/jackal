#include "Recorder.h"
#include "Log.h"

Recorder::Recorder(AudioPlaySdWav &playWav, AudioRecordQueue &queue)
    : playWav1(playWav), queue1(queue)
{
  lastSDOperation = 0;
  recordingStartTime = 0;
  currentFilename[0] = '\0'; // Initialize to empty string
  // Initialize with latest file if available
  if (!seek())
  {
    LOG_RECORDER_MSG("No WAV files found during initialization");
  }
}

void Recorder::startRecording()
{
  LOG_RECORDER_MSG("startRecordingSD");
  if (state == State::RECORDING)
  {
    return;
  }
  char filename[30];
  time_t now = Teensy3Clock.get();
  snprintf(filename, sizeof(filename), "RECORD_%04d%02d%02d_%02d%02d%02d.WAV",
           year(now), month(now), day(now), hour(now), minute(now), second(now));

  if (SD.exists((String(basePath) + filename).c_str()))
  {
    LOG_RECORDER_MSG("Remove existing file");
    SD.remove((String(basePath) + filename).c_str());
  }

  frec = SD.open((String(basePath) + filename).c_str(), FILE_WRITE);
  if (frec)
  {
    LOG_RECORDER_MSG("Starting audio capture...");
    writeWavHeader();
    queue1.begin();
    state = State::RECORDING;
    recordingStartTime = millis();
  }
}

void Recorder::continueRecording()
{
  if (queue1.available() >= 2)
  {
    byte buffer[512];

    if (AudioMemoryUsage() > 90)
    {
      LOG_RECORDER_MSG("Warning: High memory usage, flushing queue");
      while (queue1.available() > 2)
      {
        queue1.readBuffer();
        queue1.freeBuffer();
      }
    }

    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer + 256, queue1.readBuffer(), 256);
    queue1.freeBuffer();

    elapsedMicros usec = 0;
    if (frec)
    {
      if (!frec.write(buffer, 512))
      {
        LOG_RECORDER_MSG("SD write failed");
        stopRecording(); // Safely stop recording on write failure
        return;
      }
    }
    yield(); // Allow I2C processing

    if (usec > 3000)
    {
      LOG_RECORDER_MSG("Long SD write: ");
      LOG_RECORDER_MSG(usec);
    }
  }
}

void Recorder::stopRecording()
{
  LOG_RECORDER_MSG("stopRecordingSD");

  if (state != State::RECORDING)
  {
    return;
  }

  // First stop the queue from receiving new data
  queue1.end();
  state = State::STOPPED; // Mark as stopped early to prevent re-entry

  // Process remaining audio in queue
  while (queue1.available() > 0)
  {
    // Get the buffer
    byte *buffer = (byte *)queue1.readBuffer();
    if (buffer != nullptr)
    {
      frec.write(buffer, 256);
      queue1.freeBuffer();
    }
    yield(); // Give other processes a chance to run
  }

  // Update WAV header with final file size
  if (frec)
  {
    uint32_t fileSize = frec.size();
    if (fileSize > 44)
    { // Make sure we have more than just the header
      frec.seek(0);
      updateWavHeader(fileSize);

      // Ensure all data is written
      frec.flush();

      // Close the file
      frec.close();
    }
  }

  // Reset state
  recordingStartTime = 0;

  // Memory cleanup
  AudioMemoryUsageMaxReset();

  // Wait for cooldown after closing file
  elapsedMillis cooldownTimer = 0;
  while (cooldownTimer < SD_COOLDOWN_MS)
  {
    yield();
  }
  lastSDOperation = millis();

  LOG_RECORDER_MSG("Recording stopped successfully");
}

void Recorder::play()
{
  playFile(currentFilename);
}

void Recorder::playFile(const char *filename)
{
  LOG_RECORDER_MSG("playFile");
  LOG_RECORDER_MSG(filename);

  // Enforce cooldown period
  while (millis() - lastSDOperation < SD_COOLDOWN_MS)
  {
    yield(); // Allow other processes to run during wait
  }

  String filePath = String(basePath) + filename;
  if (SD.exists(filePath.c_str()))
  {
    // Stop current playback and wait for it to complete
    playWav1.stop();
    state = State::STOPPED;

    // Clear any remaining audio buffers
    AudioMemoryUsageMaxReset();

    if (playWav1.play(filePath.c_str()))
    {
      strncpy(currentFilename, filename, sizeof(currentFilename) - 1);
      currentFilename[sizeof(currentFilename) - 1] = '\0';
      state = State::PLAYING;
      playStartDebounce = 0; // playWav1.isPlaying() takes a while to return true so we need to debounce
    }
    else
    {
      LOG_RECORDER_MSG("playFile failed");
    }
  }
  else
  {
    LOG_RECORDER_MSGF("File not found: %s", filePath.c_str());
  }

  lastSDOperation = millis();
}

// bool Recorder::seekFile(bool older, bool jumpToEnd)
// {
//   lastSDOperation = millis();
//   File dir = SD.open(basePath.c_str());
//   if (!dir)
//   {
//     return false;
//   }

//   const int MAX_FILES = 100;
//   char *fileList[MAX_FILES];
//   int numFiles = listSortedFiles(fileList, MAX_FILES);

//   // Print sorted files
//   for (int i = 0; i < numFiles; i++)
//   {
//     Serial.println(fileList[i]);
//   }

//   // Clean up
//   for (int i = 0; i < numFiles; i++)
//   {
//     free(fileList[i]);
//   }

//   char bestMatch[255] = "";
//   char newestFile[255] = "";
//   char oldestFile[255] = "";
//   bool foundCurrent = false;

//   // First pass: find current file and track newest/oldest files
//   while (File entry = dir.openNextFile())
//   {
//     if (!entry.isDirectory() &&
//         (strstr(entry.name(), ".WAV") || strstr(entry.name(), ".wav")))
//     {
//       // Keep track of the chronological first (newest) and last (oldest) files
//       if (newestFile[0] == '\0' || strcmp(entry.name(), newestFile) > 0)
//       {
//         strncpy(newestFile, entry.name(), sizeof(newestFile) - 1);
//       }
//       if (oldestFile[0] == '\0' || strcmp(entry.name(), oldestFile) < 0)
//       {
//         strncpy(oldestFile, entry.name(), sizeof(oldestFile) - 1);
//       }

//       if (strcmp(entry.name(), currentFilename) == 0)
//       {
//         foundCurrent = true;
//       }
//       entry.close();
//     }
//   }

//   if (jumpToEnd || !foundCurrent)
//   {
//     if (older)
//     {
//       strncpy(currentFilename, oldestFile, sizeof(currentFilename));
//       currentFilename[sizeof(currentFilename) - 1] = '\0';
//       return oldestFile[0] != '\0'; // Return true if we found any files
//     }
//     else
//     {
//       strncpy(currentFilename, newestFile, sizeof(currentFilename));
//       currentFilename[sizeof(currentFilename) - 1] = '\0';
//       return newestFile[0] != '\0'; // Return true if we found any files
//     }
//   }

//   // // If we're not currently playing anything, return the newest file
//   // if (!foundCurrent)
//   // {
//   //   strncpy(currentFilename, newestFile, sizeof(currentFilename));
//   //   currentFilename[sizeof(currentFilename) - 1] = '\0';
//   //   return newestFile[0] != '\0'; // Return true if we found any files
//   // }

//   // Reset directory
//   dir.rewindDirectory();

//   // Second pass: find target file
//   while (File entry = dir.openNextFile())
//   {
//     if (!entry.isDirectory() && (strstr(entry.name(), ".WAV") || strstr(entry.name(), ".wav")))
//     {
//       if (older)
//       {
//         // Looking for next older file (lower timestamp)
//         if (strcmp(entry.name(), currentFilename) < 0 &&
//             (bestMatch[0] == '\0' || strcmp(entry.name(), bestMatch) > 0))
//         {
//           strncpy(bestMatch, entry.name(), sizeof(bestMatch));
//           bestMatch[sizeof(bestMatch) - 1] = '\0';
//         }
//       }
//       else
//       {
//         // Looking for next newer file (higher timestamp)
//         if (strcmp(entry.name(), currentFilename) > 0 &&
//             (bestMatch[0] == '\0' || strcmp(entry.name(), bestMatch) < 0))
//         {
//           strncpy(bestMatch, entry.name(), sizeof(bestMatch));
//           bestMatch[sizeof(bestMatch) - 1] = '\0';
//         }
//       }
//       entry.close();
//     }
//   }
//   dir.close();

//   // Handle wrap-around
//   if (bestMatch[0] == '\0')
//   {
//     if (older)
//     { // If seeking older and no older files, wrap to newest
//       strncpy(bestMatch, newestFile, sizeof(bestMatch));
//       bestMatch[sizeof(bestMatch) - 1] = '\0';
//     }
//     else
//     { // If seeking newer and no newer files, wrap to oldest
//       strncpy(bestMatch, oldestFile, sizeof(bestMatch));
//       bestMatch[sizeof(bestMatch) - 1] = '\0';
//     }
//   }

//   if (bestMatch[0] != '\0')
//   {
//     strncpy(currentFilename, bestMatch, sizeof(currentFilename));
//     currentFilename[sizeof(currentFilename) - 1] = '\0';
//     return true;
//   }

//   return false;
// }

void Recorder::playAdjacentFile(bool previous, bool wrap)
{
  // Stop current playback first
  // if (isPlaying())
  // {
  //   stopPlaying();
  //   // Wait for cooldown after stopping
  //   elapsedMillis cooldownTimer = 0;
  //   while (cooldownTimer < SD_COOLDOWN_MS)
  //   {
  //     yield();
  //   }
  // }

  LOG_RECORDER_MSGF("Seek, from file: %s", currentFilename);
  if (seek(previous, wrap))
  {
    LOG_RECORDER_MSGF("to file: %s", currentFilename);
    play();
  }
  else
  {
    LOG_RECORDER_MSG("No next file");
    stopPlaying();
  }
}

void Recorder::playNextFile(bool wrap)
{
  playAdjacentFile(false, wrap);
}

void Recorder::playPrevFile(bool wrap)
{
  playAdjacentFile(true, wrap);
}

void Recorder::continuePlaying(bool andStop)
{
  if (!playWav1.isPlaying())
  {
    // Add debounce after starting a new file
    if (playStartDebounce < 500)
    { // 500ms debounce
      return;
    }

    LOG_RECORDER_MSG("End of file reached");
    playWav1.stop();
    lastSDOperation = millis();
    state = State::STOPPED;
    if (andStop)
    {
      return;
    }
    LOG_RECORDER_MSG("Playing next file");
    playNextFile();
    playStartDebounce = 0; // Reset debounce timer
  }
}

void Recorder::stopPlaying()
{
  LOG_RECORDER_MSG("stopPlayingSD");
  if (state == State::PLAYING)
  {
    playWav1.stop();
    AudioMemoryUsageMaxReset();
  }
  state = State::STOPPED;
}

void Recorder::stopAll()
{
  stopRecording();
  stopPlaying();
}

void Recorder::parseTimestampFromFilename(const char *filename, char *dateStr, char *timeStr)
{
  if (strlen(filename) < 24)
  {
    strcpy(dateStr, "Invalid Date");
    strcpy(timeStr, "Invalid Time");
    return;
  }

  char yearStr[5] = {filename[7], filename[8], filename[9], filename[10], '\0'};
  char monthStr[3] = {filename[11], filename[12], '\0'};
  char dayStr[3] = {filename[13], filename[14], '\0'};
  char hourStr[3] = {filename[16], filename[17], '\0'};
  char minStr[3] = {filename[18], filename[19], '\0'};
  char secStr[3] = {filename[20], filename[21], '\0'};

  snprintf(dateStr, 12, "%s/%s/%s", dayStr, monthStr, yearStr);
  snprintf(timeStr, 9, "%s:%s:%s", hourStr, minStr, secStr);
}

bool Recorder::deleteCurrentFile()
{
  if (strlen(currentFilename) == 0)
  {
    return false;
  }

  if (isPlaying())
  {
    stopPlaying();
  }

  LOG("Deleting file: ");
  LOG(currentFilename);

  char oldFilename[255];
  strncpy(oldFilename, currentFilename, sizeof(oldFilename));

  bool success = SD.remove((String(basePath) + oldFilename).c_str());
  if (success)
  {
    seek(); // Find next file after deletion
  }
  return success;
}

void Recorder::getMetadata(char *line1, char *line2)
{
  if (isRecording())
  {
    unsigned long duration = (millis() - recordingStartTime) / 1000; // Convert to seconds
    strncpy(line1, "Recording", 31);
    snprintf(line2, 32, "%02lu:%02lu", duration / 60, duration % 60);
  }
  else
  {
    parseTimestampFromFilename(currentFilename, line1, line2);
  }
  line1[31] = '\0';
  line2[31] = '\0';
}

void Recorder::writeWavHeader()
{
  unsigned char header[44];
  unsigned int sampleRate = 44100;
  unsigned int channels = 1;       // Mono
  unsigned int bitsPerSample = 16; // 16-bit audio

  // RIFF chunk
  memcpy(header + 0, "RIFF", 4);
  *(uint32_t *)(header + 4) = 0; // Final size unknown, will update later
  memcpy(header + 8, "WAVE", 4);

  // fmt chunk
  memcpy(header + 12, "fmt ", 4);
  *(uint32_t *)(header + 16) = 16; // fmt chunk size
  *(uint16_t *)(header + 20) = 1;  // PCM format
  *(uint16_t *)(header + 22) = channels;
  *(uint32_t *)(header + 24) = sampleRate;
  *(uint32_t *)(header + 28) = sampleRate * channels * (bitsPerSample / 8); // Byte rate
  *(uint16_t *)(header + 32) = channels * (bitsPerSample / 8);              // Block align
  *(uint16_t *)(header + 34) = bitsPerSample;

  // data chunk
  memcpy(header + 36, "data", 4);
  *(uint32_t *)(header + 40) = 0; // Data size unknown, will update later

  frec.write(header, 44);
}

void Recorder::updateWavHeader(uint32_t fileSize)
{
  uint32_t dataSize = fileSize - 44;
  uint32_t riffSize = fileSize - 8;

  // Update RIFF chunk size
  frec.seek(4);
  frec.write((const uint8_t *)&riffSize, 4);

  // Update data chunk size
  frec.seek(40);
  frec.write((const uint8_t *)&dataSize, 4);
}

bool Recorder::seekToFirstFile()
{
  stopPlaying();
  currentFilename[0] = '\0';
  return seek();
}

// Sort the array using qsort
int Recorder::compareFileNames(const void *a, const void *b)
{
  return strcmp(*(const char **)a, *(const char **)b);
}

// Returns number of files found, fills array of pointers
int Recorder::listSortedFiles(char **fileList, int maxFiles)
{
  File dir = SD.open(basePath.c_str());
  if (!dir)
  {
    return 0;
  }

  int fileCount = 0;

  while (File entry = dir.openNextFile())
  {
    if (entry.isDirectory() || strcmp(entry.name(), ".") == 0 || strcmp(entry.name(), "..") == 0)
    {
      continue;
    }
    // Allocate memory for the filename and copy it
    fileList[fileCount] = strdup(entry.name());
    if (fileList[fileCount] != NULL)
    {
      fileCount++;
    }
  }

  dir.close();

  qsort(fileList, fileCount, sizeof(char *), compareFileNames);

  return fileCount;
}

bool Recorder::seek(bool previous, bool wrap)
{
  const int MAX_FILES = 100;
  char *fileList[MAX_FILES];
  int numFiles = listSortedFiles(fileList, MAX_FILES);

  if (numFiles == 0)
  {
    return false;
  }

  // Find current file index
  int currentIndex = -1;
  for (int i = 0; i < numFiles; i++)
  {
    if (strcmp(fileList[i], currentFilename) == 0)
    {
      currentIndex = i;
      break;
    }
  }

  // Get next (or previous) file
  int nextIndex = currentIndex + (previous ? -1 : 1) * (reverseAlphabeticalOrder ? -1 : 1);
  LOG_RECORDER_MSGF("current: %d, nextIndex: %d", currentIndex, nextIndex);
  bool success = false;
  if (nextIndex < 0 || nextIndex >= numFiles)
  {
    if (wrap)
    {
      strncpy(currentFilename, fileList[reverseAlphabeticalOrder ? numFiles - 1 : 0], sizeof(currentFilename));
      currentFilename[sizeof(currentFilename) - 1] = '\0';
      success = true;
    }
  }
  else
  {
    strncpy(currentFilename, fileList[nextIndex], sizeof(currentFilename));
    currentFilename[sizeof(currentFilename) - 1] = '\0';
    success = true;
  }

  // Clean up
  for (int i = 0; i < numFiles; i++)
  {
    free(fileList[i]);
  }

  return success;
}
