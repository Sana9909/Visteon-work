# ============================================================
# Vehicle ECU Simulator - Root Makefile
# Builds lib/libecu.a then links with main.c -> ecu_sim.exe
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -pedantic -O2
TARGET  = ecu_sim.exe

all: lib_build $(TARGET)
	@echo         Build successful.

lib_build:
	$(MAKE) -C lib

$(TARGET): main.c lib/libecu.a
	$(CC) $(CFLAGS) -Ilib -o $@ main.c lib/libecu.a

run: $(TARGET)
	@echo [3/4] Running simulation...
	@echo 1 | .\$(TARGET)
	@echo         Output saved to log.txt

clean:
	$(MAKE) -C lib clean
	@del /Q $(TARGET) _objtmp\*.o 2>NUL || true

.PHONY: all lib_build run clean
