CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRC = src/main.c src/server.c src/http.c src/cache.c
TARGET = webserver

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

run: $(TARGET)
	./$(TARGET) 8080

clean:
	rm -f $(TARGET)