#pragma once
#include "rom.h"
#include "ppu3.h"
#include "apu.h"
#include "util.h"
#include<string>

class apu;

class mem {

public:
    mem(rom & romi,ppu & ppui,apu & apui);
    ~mem();
    void set_cycle(const unsigned int cycle);
    void set_frame(const unsigned int frame);
    const unsigned int read(const unsigned int address);
    const unsigned int read2(const unsigned int);
    const unsigned int get_page(const unsigned int);
    void write(const unsigned int address, const unsigned char val);
    const unsigned int get_rst_addr();
    const unsigned int get_nmi_addr();
    const unsigned int get_irq_addr();
    void sendkeydown(SDL_Scancode test);
    void sendkeyup(SDL_Scancode test);
    void sendmousedown(const int x, const int y);
    void sendmouseup(const int x, const int y);
    void sendmousepos(const int x, const int y);
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
    int joy2_bit;
    int joy2_trigger;
    int joy2_light;
    int mouse_x;
    int mouse_y;
    std::bitset<8> joy1_buttons;
    std::bitset<8> joy2_buttons;
    Vect<int> read_from;
    Vect<int> written_to;
};
