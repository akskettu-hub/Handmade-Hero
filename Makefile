CC = gcc

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g
LDFLAGS = -lX11

TARGET = build/handmade

SOURCES = \
	src/main.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
