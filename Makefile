CC = gcc

CFLAGS = -std=gnu11 -Wall -Wextra -Wpedantic -g
LDFLAGS = -lX11

TARGET = build/handmade
AUDIO_TEST = build/audio_test

SOURCES = \
	src/main.c \
	src/platform_audio.c

all: $(TARGET) $(AUDIO_TEST)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS) -lasound -lm

$(AUDIO_TEST): src/audio_test.c
	$(CC) $(CFLAGS) src/audio_test.c -o $(AUDIO_TEST) -lasound -lm

run: $(TARGET)
	./$(TARGET)

audio: $(AUDIO_TEST)
	./$(AUDIO_TEST)

clean:
	rm -f $(TARGET) $(AUDIO_TEST)
