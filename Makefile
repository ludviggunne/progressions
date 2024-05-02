
CC=clang
CFLAGS=$(shell paste -sd " " compile_flags.txt)

.PHONY: clean

track.mid: main
	./$< > $@

main: main.c definitions.h
	$(CC) $(CFLAGS) -o $@ main.c

clean:
	rm -fr main *.mid
