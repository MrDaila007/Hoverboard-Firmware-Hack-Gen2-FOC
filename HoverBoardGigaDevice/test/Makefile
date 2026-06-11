CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Werror -I../Inc
LDFLAGS = -lm

SRCS = test_bldc_sinusoidal.c ../Src/bldc_sinusoidal.c
TARGET = test_bldc_sinusoidal

.PHONY: all test clean

all: test

test: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)
