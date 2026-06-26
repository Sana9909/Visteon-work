# ============================================================
# lib/Makefile — Builds static library libecu.a
# Also generates lib_sizes.txt with per-module size report
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -pedantic -Os -I.

SRCS    = ecu_control.c input.c mode.c control.c fault.c state.c log.c perf.c system_io.c
OBJS    = $(SRCS:.c=.o)
LIB     = libecu.a

all: $(LIB)
	@echo === Module Object Sizes (text/data/bss in bytes) === > ../lib_sizes.txt
	@size $(OBJS) >> ../lib_sizes.txt
	@echo. >> ../lib_sizes.txt
	@echo === Static Library (libecu.a) === >> ../lib_sizes.txt
	@size $(LIB) >> ../lib_sizes.txt
	@echo. >> ../lib_sizes.txt
	@echo === File Sizes === >> ../lib_sizes.txt
	@for %%f in ($(OBJS)) do @echo %%f: %%~zf bytes >> ../lib_sizes.txt
	@for %%f in ($(LIB)) do @echo %%f: %%~zf bytes >> ../lib_sizes.txt
	@del /Q $(OBJS) 2>NUL || true

$(LIB): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@del /Q $(OBJS) $(LIB) 2>NUL || true

.PHONY: all clean
