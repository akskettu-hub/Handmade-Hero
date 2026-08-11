#include "platform_audio.h"

#include <alsa/asoundlib.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
  int16_t *buffer; // NOTE: The buffer pointed to by this needs to be allocated!

  int capacity;       // Number of frames the buffer can hold
  int read_position;  // Where ALSA will get its next frame
  int write_position; // where the game will put its next frame
  int count;          // number of frames currently in the buffer
} AudioRingBuffer;

struct AudioState { // represents audio device and its current state
  snd_pcm_t *pcm;

  int sample_rate;
  int channels;

  double phase;
  double frequency;

  // int16_t *buffer;
  AudioRingBuffer ring;

  snd_pcm_uframes_t buffer_size;
  snd_pcm_uframes_t period_size;
};

#define AUDIO_RING_FRAMES 48000
#define ONE_SEC_AUDIO 48000

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

  int16_t *audio_ring_memory =
      malloc(AUDIO_RING_FRAMES * audio->channels * sizeof(int16_t));

  if (!audio_ring_memory) {
    fprintf(stderr, "Could not allocate ring memory\n");
  }

  audio->ring.buffer = audio_ring_memory;
  audio->ring.capacity = AUDIO_RING_FRAMES;
  audio->ring.read_position = 0;
  audio->ring.write_position = 0;
  audio->ring.count = 0;

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

  result =
      snd_pcm_get_params(audio->pcm, &audio->buffer_size, &audio->period_size);

  if (result < 0) {
    fprintf(stderr, "Could not get pmc buffer or period size: %s\n",
            snd_strerror(result));
    return NULL;
  }
  return audio;
}

void audio_shutdown(AudioState *audio) {
  if (audio->pcm) {
    snd_pcm_drain(audio->pcm);
    snd_pcm_close(audio->pcm);
    // audio->pcm = NULL;
    free(audio->ring.buffer);
    free(audio);
  }
}

int audio_ring_write(AudioRingBuffer *ring, int16_t *source, int frames) {
  int frames_written = 0;

  while (frames_written < frames && ring->count < ring->capacity) {
    int write = ring->write_position;

    ring->buffer[write * 2 + 0] = source[frames_written * 2 + 0];
    ring->buffer[write * 2 + 1] = source[frames_written * 2 + 1];

    ring->write_position++;

    if (ring->write_position >= ring->capacity) {
      ring->write_position = 0;
    }
    ring->count++;
    frames_written++;
  }
  return frames_written;
}

int audio_ring_read(AudioRingBuffer *ring, int16_t *destination, int frames) {
  int frames_read = 0;

  while (frames_read < frames && ring->count > 0) {
    int read = ring->read_position;

    destination[frames_read * 2 + 0] = ring->buffer[read * 2 + 0];
    destination[frames_read * 2 + 1] = ring->buffer[read * 2 + 1];

    ring->read_position++;

    if (ring->read_position >= ring->capacity) {
      ring->read_position = 0;
    }

    ring->count--;
    frames_read++;
  }
  return frames_read;
}

#define TARGET 4800
#define AUDIO_TEMP_BUFFER_FRAMES 4800
#define TEST_FRAMES 1024
#define AUDIO_GENERATE_BUFFER_FRAMES 4096
#define AUDIO_OUTPUT_BUFFER_FRAMES 4096

void audio_update(AudioState *audio, int16_t *temp_buffer,
                  int16_t *temp_output_buffer) {
  snd_pcm_sframes_t available = snd_pcm_avail_update(audio->pcm);

  long queued = audio->buffer_size - available;

  int target = audio->sample_rate / 10;

  if (audio->ring.count < target) {
    int frames_to_generate = target - audio->ring.count;

    if (frames_to_generate > AUDIO_GENERATE_BUFFER_FRAMES) {

      frames_to_generate = AUDIO_GENERATE_BUFFER_FRAMES;
    }
    audio_generate(audio, temp_buffer, frames_to_generate);

    int frames_written =
        audio_ring_write(&audio->ring, temp_buffer, frames_to_generate);

    if (frames_written != frames_to_generate) {
      fprintf(stderr, "Ring buffer could not accept all generated audio"

      );
    }
  }

  int written_pcm = audio_output(audio, temp_output_buffer);
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

int audio_output(AudioState *audio, int16_t *output_buffer) {
  snd_pcm_sframes_t available = snd_pcm_avail_update(audio->pcm);

  if (available <= 0) {
    return 1;
  }

  int frames_to_output = available;

  if (frames_to_output > AUDIO_OUTPUT_BUFFER_FRAMES) {
    frames_to_output = AUDIO_OUTPUT_BUFFER_FRAMES;
  }

  if (frames_to_output > audio->ring.count) {
    frames_to_output = audio->ring.count;
  }

  if (frames_to_output == 0) {
    return 1;
  }

  int frames_read =
      audio_ring_read(&audio->ring, output_buffer, frames_to_output);

  if (frames_read > 0) {
    snd_pcm_sframes_t written =
        snd_pcm_writei(audio->pcm, output_buffer, frames_read);

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
  }

  return 1;
}
