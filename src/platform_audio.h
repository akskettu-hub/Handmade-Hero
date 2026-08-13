#ifndef PLATFORM_AUDIO_H
#define PLATFORM_AUDIO_H

// #include <cstdint>
#include <alsa/asoundlib.h>
#include <stdint.h>

#define AUDIO_GENERATE_BUFFER_FRAMES 4096
#define AUDIO_OUTPUT_BUFFER_FRAMES 4096

// NOTE: These were moved here temporarily to give game audio info about sound
// setup
// typedef struct AudioState AudioState;

typedef struct {
  int16_t *buffer; // NOTE: The buffer pointed to by this needs to be allocated!

  int capacity;       // Number of frames the buffer can hold
  int read_position;  // Where ALSA will get its next frame
  int write_position; // where the game will put its next frame
  int count;          // number of frames currently in the buffer

  int target; // N frames to keep in buffer
} AudioRingBuffer;

typedef struct AudioState { // represents audio device and its current state
  snd_pcm_t *pcm;

  int sample_rate;
  int channels;

  double phase;
  double frequency;
  int16_t toneVolume;

  AudioRingBuffer ring;

  snd_pcm_uframes_t buffer_size;
  snd_pcm_uframes_t period_size;
} AudioState;

AudioState *audio_init(void);
void audio_generate(AudioState *audio, int16_t *buffer, int frames);
int audio_output(AudioState *audio, int16_t *output_buffer);
void audio_update(AudioState *audio, int16_t *temp_buffer,
                  int16_t *temp_output_buffer, int frames_to_generate);
void audio_shutdown(AudioState *audio);
int linuxAudioRequestedFrames(AudioState *audio);

#endif
