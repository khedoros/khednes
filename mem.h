#ifndef MEM_H
#define MEM_H

#include "rom.h"
#include "ppu3.h"
#include "apu.h"
#include "util.h"
#include<string>

class mem {

public:
    mem(rom & romi,ppu & ppui,apu & apui);
    ~mem();
    void set_cycle(unsigned int cycle);
    void set_frame(unsigned int frame);
    const unsigned int read(unsigned int address);
    const unsigned int read2(unsigned int);
    void write(unsigned int address, unsigned char val);
    const unsigned int get_rst_addr();
    const unsigned int get_nmi_addr();
    const unsigned int get_irq_addr();
    void sendkeydown(SDL_Scancode test);
    void sendkeyup(SDL_Scancode test);
    const unsigned int get_ppu_cycles(const unsigned int frame);

private:
    rom & cart;
    ppu & pu;
    apu & snd;
    unsigned int cycle;
    Vect<unsigned char> ram;
    Vect<unsigned char> sram;
    unsigned int frame;
    bool joy1_strobe;
    int joy1_bit;
    std::bitset<8> joy1_buttons;
};
#endif