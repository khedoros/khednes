.PHONY: main

GCC_VERSION:=$(shell g++ -v 2>&1 |grep gcc\ version | cut -d' ' -f3)
GCC_MAJOR:=$(shell echo "$(GCC_VERSION)" | cut -d'.' -f1)
GCC_MINOR:=$(shell echo "$(GCC_VERSION)" | cut -d'.' -f2)
GCC_SUB:=$(shell echo "$(GCC_VERSION)" | cut -d'.' -f3)

MAIN_OPTS = -Wall -std=c++11 -O3 -I/usr/local/include -L/usr/local/lib
DBG_OPTS = $(MAIN_OPTS) -DDEBUG -pg -g -O0
SDL2_LIBS = `sdl2-config --libs --cflags`

main:
	#touch *.cpp
	make -j 4 khednes
#	touch *.cpp
#	make debug

#.PHONY: khednes
#khednes: main.o rom.o mem.o cpu.o ppu.o util.o apu.o sq_chan.o
#	g++ $(MAIN_OPTS) -lSDL $^ -o khednes


.PHONY: khednes
khednes: main.o rom.o mem.o cpu2.o ppu3.o util.o apu.o compositor.o mapper.o mapper_001_mmc1.o mapper_002_unrom.o mapper_003_cnrom.o mapper_011_color_dreams.o mapper_nsf.o
	g++ $(MAIN_OPTS) $^ -o khednes $(SDL2_LIBS)

#.PHONY: debug
#debug: main.odb rom.odb mem.odb cpu.odb ppu.odb util.odb apu.odb sq_chan.odb
#	g++ $(DBG_OPTS) $^ -lSDL -o khednes-dbg

.PHONY: debug
debug: main.odb rom.odb mem.odb cpu2.odb ppu3.odb util.odb apu.odb compositor.odb mapper.odb mapper_001_mmc1.odb mapper_002_unrom.odb mapper_003_cnrom.odb mapper_011_color_dreams.odb mapper_nsf.odb
	g++ $(DBG_OPTS) $^ -o khednes-dbg $(SDL2_LIBS)

main.o: main.cpp
	g++ -c $(MAIN_OPTS) $<

main.odb: main.cpp
	g++ -c $(DBG_OPTS) $< -o main.odb

%.o:%.cpp %.h
	g++ -c $(MAIN_OPTS) $<

%.odb:%.cpp %.h
	g++ -c $(DBG_OPTS) $< -o $*.odb

.PHONY: clean
clean:
	@-rm *.o *.odb khednes khednes-dbg
