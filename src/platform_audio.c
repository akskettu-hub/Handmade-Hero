#include "platform_audio.h"

#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define AUDIO_RING_FRAMES 48000

typedef struct {
  int16_t *buffer; // NOTE: The buffer pointed to by this needs to be allocated!

  int capacity;      // Number of frames the buffer can hold
  int readPosition;  // Where ALSA will get its next frame
  int writePosition; // where the game will put its next frame
  int count;         // number of frames currently in the buffer

  int target; // N frames to keep in ring buffer
} AudioRingBuffer;

struct AudioState { // represents platform audio device and its state
  snd_pcm_t *pcm;

  int sampleRate;
  int channels;

  AudioRingBuffer ring;

  snd_pcm_uframes_t bufferSize;
  snd_pcm_uframes_t periodSize;
};

AudioState *audioInit(void) {
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

  audio->sampleRate = AUDIO_SAMPLE_RATE;
  audio->channels = AUDIO_CHANNELS;

  int16_t *audioRingMemory =
      malloc(AUDIO_RING_FRAMES * audio->channels * sizeof(int16_t));

  if (!audioRingMemory) {
    fprintf(stderr, "Could not allocate ring memory\n");
  }

  audio->ring.buffer = audioRingMemory;
  audio->ring.capacity = AUDIO_RING_FRAMES;
  audio->ring.readPosition = 0;
  audio->ring.writePosition = 0;
  audio->ring.count = 0;
  audio->ring.target = audio->sampleRate / 10;

  result = snd_pcm_set_params(audio->pcm, SND_PCM_FORMAT_S16_LE,
                              SND_PCM_ACCESS_RW_INTERLEAVED, audio->channels,
                              audio->sampleRate, 1, 500000);

  if (result < 0) {
    fprintf(stderr, "Could not configure audio device: %s\n",
            snd_strerror(result));
    snd_pcm_close(audio->pcm);
    audio->pcm = NULL;
    free(audio);

    return NULL;
  }

  result =
      snd_pcm_get_params(audio->pcm, &audio->bufferSize, &audio->periodSize);

  if (result < 0) {
    fprintf(stderr, "Could not get pmc buffer or period size: %s\n",
            snd_strerror(result));
    return NULL;
  }
  return audio;
}

void audioShutdown(AudioState *audio) {
  if (audio->pcm) {
    snd_pcm_drain(audio->pcm);
    snd_pcm_close(audio->pcm);
    // audio->pcm = NULL;
    free(audio->ring.buffer);
    free(audio);
  }
}

int audioRingWrite(AudioRingBuffer *ring, int16_t *source, int frames) {
  int framesWritten = 0;

  while (framesWritten < frames && ring->count < ring->capacity) {
    int write = ring->writePosition;

    ring->buffer[write * 2 + 0] = source[framesWritten * 2 + 0];
    ring->buffer[write * 2 + 1] = source[framesWritten * 2 + 1];

    ring->writePosition++;

    if (ring->writePosition >= ring->capacity) {
      ring->writePosition = 0;
    }
    ring->count++;
    framesWritten++;
  }
  return framesWritten;
}

int audioRingRead(AudioRingBuffer *ring, int16_t *destination, int frames) {
  int framesRead = 0;

  while (framesRead < frames && ring->count > 0) {
    int read = ring->readPosition;

    destination[framesRead * 2 + 0] = ring->buffer[read * 2 + 0];
    destination[framesRead * 2 + 1] = ring->buffer[read * 2 + 1];

    ring->readPosition++;

    if (ring->readPosition >= ring->capacity) {
      ring->readPosition = 0;
    }

    ring->count--;
    framesRead++;
  }
  return framesRead;
}

int linuxAudioRequestedFrames(AudioState *audio) {
  // snd_pcm_sframes_t available = snd_pcm_avail_update(audio->pcm);
  //  long queued = audio->buffer_size - available;

  int target = audio->ring.target;

  if (audio->ring.count < target) {
    int framesToGenerate =
        target - audio->ring.count; // remaining to get to target

    if (framesToGenerate > AUDIO_GENERATE_BUFFER_FRAMES) {

      framesToGenerate = AUDIO_GENERATE_BUFFER_FRAMES;
    }

    return framesToGenerate;
  }
  return 0;
}

int audioOutput(AudioState *audio, int16_t *output_buffer) {
  snd_pcm_sframes_t available = snd_pcm_avail_update(audio->pcm);

  if (available <= 0) {
    return 1;
  }

  int framesToOutput = available;

  if (framesToOutput > AUDIO_OUTPUT_BUFFER_FRAMES) {
    framesToOutput = AUDIO_OUTPUT_BUFFER_FRAMES;
  }

  if (framesToOutput > audio->ring.count) {
    framesToOutput = audio->ring.count;
  }

  if (framesToOutput == 0) {
    return 1;
  }

  int framesRead = audioRingRead(&audio->ring, output_buffer, framesToOutput);

  if (framesRead > 0) {
    snd_pcm_sframes_t written =
        snd_pcm_writei(audio->pcm, output_buffer, framesRead);

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

void audioUpdate(AudioState *audio, int16_t *tempBuffer,
                 int16_t *tempOutputBuffer, int framesToGenerate) {
  // int frames_to_generate = linuxAudioRequestedFrames(audio);
  if (framesToGenerate) {
    // audio_generate(audio, temp_buffer, frames_to_generate);

    int framesWritten =
        audioRingWrite(&audio->ring, tempBuffer, framesToGenerate);

    if (framesWritten != framesToGenerate) {
      fprintf(stderr, "Ring buffer could not accept all generated audio");
    }
  }
  audioOutput(audio, tempOutputBuffer);
}
