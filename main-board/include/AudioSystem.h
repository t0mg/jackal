#pragma once

#include <Audio.h>
#include "async_input.h"
#include "input_i2s2_16bit.h"

class AudioSystem {
public:
    AudioSystem();
    void init();

    void setBandValue(int band, float value);
    float getBandValue(int band) const;
    // Getters for audio components
    AudioPlayMemory* getMemoryPlayer() { return &playMem1; }
    AudioPlaySdWav* getWavPlayer() { return &playSdWav1; }
    AudioRecordQueue* getRecordQueue() { return &queue1; }
    AudioAnalyzePeak* getPlaybackPeak() { return &playbackPeak; }
    AudioAnalyzePeak* getRecorderPeak() { return &recorderPeak; }
    AudioAnalyzeFFT1024* getFFT() { return &fft1024_1; }
    AudioControlSGTL5000* getCodec() { return &sgtl5000_1; }
    AudioAmplifier* getOutputAmp() { return &outputAmp; }
    AudioEffectBitcrusher* getBitcrusher() { return &bitcrusher1; }
    AudioMixer4* getMainMixer() { return &mixerMain; }
    AudioMixer4* getMonoDownmixer() { return &mixerMonoDownmix; }
    AudioMixer4* getFFTInputMixer() { return &mixerFFTInput; }
    AsyncAudioInput<AsyncAudioInputI2S2_16bitslave>* getBluetoothInput() { return &i2sBluetoothSink; }
    AudioFilterBiquad* getBiquad1() { return &biquad1; }
    AudioFilterBiquad* getBiquad2() { return &biquad2; }
    AudioAmplifier* getRecorderAmp() { return &recorderAmp; }

private:
    // Audio components
    AudioPlayMemory playMem1;
    AudioInputI2S i2sLineInput;
    AudioPlaySdWav playSdWav1;
    AsyncAudioInput<AsyncAudioInputI2S2_16bitslave> i2sBluetoothSink;
    AudioMixer4 mixerMonoDownmix;
    AudioFilterBiquad biquad2;
    AudioMixer4 mixerMain;
    AudioAmplifier recorderAmp;
    AudioFilterBiquad biquad1;
    AudioEffectBitcrusher bitcrusher1;
    AudioAmplifier outputAmp;
    AudioMixer4 mixerFFTInput;
    AudioRecordQueue queue1;
    AudioAnalyzeFFT1024 fft1024_1;
    AudioOutputI2S i2sOutput;
    AudioAnalyzePeak playbackPeak;
    AudioAnalyzePeak recorderPeak;
    AudioControlSGTL5000 sgtl5000_1;

    // Audio connections
    AudioConnection patchCord1;
    AudioConnection patchCord2;
    AudioConnection patchCord3;
    AudioConnection patchCord4;
    AudioConnection patchCord5;
    AudioConnection patchCord6;
    AudioConnection patchCord7;
    AudioConnection patchCord8;
    AudioConnection patchCord9;
    AudioConnection patchCord10;
    AudioConnection patchCord11;
    AudioConnection patchCord12;
    AudioConnection patchCord13;
    AudioConnection patchCord14;
    AudioConnection patchCord15;
    AudioConnection patchCord16;
    AudioConnection patchCord17;
    AudioConnection patchCord18;
    AudioConnection patchCord19;
    AudioConnection patchCord20;
    AudioConnection patchCord21;

    // EQ settings
    float bandValues[5] = {0.5f, 0.3f, -0.2f, -0.1f, 0.1f};
}; 