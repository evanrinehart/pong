RAYLIB_VERSION = master
RAYLIB_PATH = vendor/raylib/src
RAYLIB_URL = https://github.com/raysan5/raylib.git

pong: raylib.h libraylib.a pong.c
	gcc -Wall -o pong -I. pong.c libraylib.a -lm

raylib :
	git clone --depth=1 --branch=$(RAYLIB_VERSION) $(RAYLIB_URL) raylib

libraylib.a : raylib
	$(MAKE) -C raylib/src
	cp raylib/src/libraylib.a .

raylib.h : raylib
	cp raylib/src/raylib.h .

