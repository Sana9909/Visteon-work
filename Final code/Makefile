CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c99 -g
TARGET  = vehicle_ecu

OBJS    = $(SRCS:.c=.o)
SRCS = main.c test_loader.c input.c mode.c control.c fault.c state.c log.c
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean