#include "AudioSystem.h"

AudioSystem::AudioSystem() : // Initialize i2sBluetoothSink parameters
                             i2sBluetoothSink(false, false, 100, 20, 80),

                             // Initialize all audio connections
                             patchCord1(playMem1, 0, mixerMain, 3),
                             patchCord2(i2sLineInput, 0, mixerMonoDownmix, 2),
                             patchCord3(i2sLineInput, 0, biquad2, 0),
                             patchCord4(i2sLineInput, 1, mixerMonoDownmix, 3),
                             patchCord5(playSdWav1, 0, mixerMain, 1),
                             patchCord6(playSdWav1, 1, mixerMain, 2),
                             patchCord7(i2sBluetoothSink, 0, mixerMonoDownmix, 0),
                             patchCord8(i2sBluetoothSink, 1, mixerMonoDownmix, 1),
                             patchCord9(mixerMonoDownmix, 0, mixerMain, 0),
                             patchCord10(biquad2, recorderAmp),
                             patchCord11(mixerMain, biquad1),
                             patchCord12(recorderAmp, queue1),
                             patchCord13(recorderAmp, recorderPeak),
                             patchCord14(recorderAmp, 0, mixerFFTInput, 1),
                             patchCord15(biquad1, bitcrusher1),
                             patchCord16(bitcrusher1, 0, mixerFFTInput, 0),
                             patchCord17(bitcrusher1, outputAmp),
                             patchCord18(outputAmp, 0, i2sOutput, 0),
                             patchCord19(outputAmp, 0, i2sOutput, 1),
                             patchCord20(outputAmp, playbackPeak),
                             patchCord21(mixerFFTInput, fft1024_1)
{
}

void AudioSystem::init()
{
  // Initialize audio memory
  AudioMemory(160);

  // Set initial mixer gains to 0 to prevent audio bleed during boot
  mixerMain.gain(0, 0.0); // BT+Radio audio
  mixerMain.gain(1, 0.0); // SD card audio L
  mixerMain.gain(2, 0.0); // SD card audio R
  mixerMain.gain(3, 1.0); // Boot sound only

  recorderAmp.gain(3.0);

  // Enable the audio shield and configure it
  sgtl5000_1.enable();
  sgtl5000_1.muteLineout();
  sgtl5000_1.volume(0.4);
  sgtl5000_1.adcHighPassFilterDisable();
  sgtl5000_1.audioPostProcessorEnable();
  sgtl5000_1.eqBands(bandValues[0],
                     bandValues[1],
                     bandValues[2],
                     bandValues[3],
                     bandValues[4]);

  // Configure biquad filters
  biquad1.setHighShelf(0, 20000, -5, 0.9);
  biquad1.setLowShelf(1, 20, -5, 0.9);

  biquad2.setHighShelf(0, 18000, -10, 0.9);
  biquad2.setLowShelf(1, 100, -4, 0.9);

  // Configure bitcrusher
  bitcrusher1.bits(16);
  bitcrusher1.sampleRate(44100);
}

void AudioSystem::setBandValue(int band, float value)
{
  if (band >= 0 && band < 5)
  {
    bandValues[band] = constrain(value, -1.0f, 1.0f);
    sgtl5000_1.eqBands(
        bandValues[0],
        bandValues[1],
        bandValues[2],
        bandValues[3],
        bandValues[4]);
  }
}

float AudioSystem::getBandValue(int band) const
{
  if (band >= 0 && band < 5)
  {
    return bandValues[band];
  }
  return 0.0f;
}