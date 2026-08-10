#include "platform_audio.h"

#include <alsa/asoundlib.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct AudioState { // represents audio device and its current state
  snd_pcm_t *pcm;

  int sample_rate;
  int channels;

  double phase; // phase lives between calls and so belongs in persistent audio
                // state
  double frequency;
};

AudioState *audio_init(void) {
  AudioState *audio = malloc(sizeof(AudioState));

  if (!audio) {
    fprintf(stderr, "Could not allocate audio state\n");
    return NULL;
  }

  int result = snd_pcm_open(&audio->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);

  if (result < 0) {
    fprintf(stderr, "Could not open audio device %s\n", snd_strerror(result));
    free(audio);
    return NULL;
  }

  audio->sample_rate = 48000;
  audio->channels = 2;
  audio->phase = 0.0;
  audio->frequency = 440.0;
  // audio->buffer_size = 4800;

  result = snd_pcm_set_params(audio->pcm, SND_PCM_FORMAT_S16_LE,
                              SND_PCM_ACCESS_RW_INTERLEAVED, audio->channels,
                              audio->sample_rate, 1, 500000);

  if (result < 0) {
    fprintf(stderr, "Could not configure audio device: %s\n",
            snd_strerror(result));
    snd_pcm_close(audio->pcm);
    audio->pcm = NULL;
    free(audio);

    return NULL;
  }

  return audio;
}

void audio_shutdown(AudioState *audio) {
  if (audio->pcm) {
    snd_pcm_drain(audio->pcm);
    snd_pcm_close(audio->pcm);
    // audio->pcm = NULL;
    free(audio);
  }
}
#define TARGET 4800

void audio_update(AudioState *audio, int16_t *buffer) {
  snd_pcm_sframes_t available = snd_pcm_avail_update(audio->pcm);
  // printf("Available: %ld\n", available);

  long queued = 24000L - available;

  // printf("Available: %ld, Queued: %ld, State: %s\n", available, queued,
  //       snd_pcm_state_name(snd_pcm_state(audio->pcm)));

  if (queued < TARGET) {

    long frames_to_generate = TARGET - queued;
    // printf("Frames to generate: %ld\n", frames_to_generate);

    audio_generate(audio, buffer, frames_to_generate);
    audio_output(audio, buffer, frames_to_generate);
  }
}

void audio_generate(AudioState *audio, int16_t *buffer, int frames) {
  for (int frame = 0; frame < frames; frame++) {
    double value = sin(audio->phase * 2.0 * M_PI);

    int16_t sample = (int16_t)(value * 3000);

    buffer[frame * 2 + 0] = sample;
    buffer[frame * 2 + 1] = sample;

    audio->phase += audio->frequency / audio->sample_rate;

    if (audio->phase >= 1.0) {
      audio->phase -= 1.0;
    }
  }
}

int audio_output(AudioState *audio, int16_t *buffer, int frames) {
  snd_pcm_sframes_t written = snd_pcm_writei(audio->pcm, buffer, frames);
  // printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(audio->pcm)));

  if (written < 0) {
    written = snd_pcm_recover(audio->pcm, written, 0);
  }

  if (written < 0) {
    fprintf(stderr, "Audio write failed %s\n", snd_strerror(written));
    return 0;
  }

  if (snd_pcm_state(audio->pcm) == SND_PCM_STATE_PREPARED) {
    int result = snd_pcm_start(audio->pcm);

    if (result < 0) {
      fprintf(stderr, "Could not start audio: %s\n", snd_strerror(result));
      return 0;
    }
  }

  return 1;
}
