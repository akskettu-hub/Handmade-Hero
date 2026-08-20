CC = gcc

CFLAGS = -std=gnu11 \
				 -Wall \
				 -Wextra \
				 -Wpedantic \
				 -g \
				 -DHANDMADE_INTERNAL=1

LDFLAGS = -lX11

TARGET = build/handmade

SOURCES = \
	src/linux_handmade.c \
	src/platform_audio.c \
	src/handmade.c

all: $(TARGET) $(AUDIO_TEST)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS) -lasound -lm

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
