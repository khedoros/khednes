#include "mapper_nsf.h"
#include<cstdio>

mapper_nsf::mapper_nsf(rom* r) : mapper(r) {
    cart = r;
    reset_map();
}
const unsigned int mapper_nsf::get_pbyte(const unsigned int addr) {
    int segment = (addr / 0x1000) - 8;
    int offset = addr % 0x1000;
    return cart->prom[bank[segment]*0x1000 + offset];
}

const unsigned int mapper_nsf::get_pword(const unsigned int addr) {
    int segment = (addr / 0x1000) - 8;
    int offset = addr % 0x1000;
    return cart->prom_w[bank[segment]*0x1000 + offset];
}

const unsigned int mapper_nsf::get_cbyte(const unsigned int addr) {
    return 0;
}

void mapper_nsf::put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr) {
    if(addr>=0x5ff8 && addr <= 0x5fff) {
        bank[addr - 0x5ff8] = val;
    }
    //printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", bank[0], bank[1], bank[2], bank[3], bank[4], bank[5], bank[6], bank[7]);
}

bool mapper_nsf::put_cbyte(const unsigned int val,const unsigned int addr) {
    return false;
}

int mapper_nsf::changed_crom() {
    return 0;
}

void mapper_nsf::ppu_change(unsigned int cycle, unsigned int addr, unsigned int val) {
}

rom::ppu_change_t mapper_nsf::cycle_forward(unsigned int cycle) {
    rom::ppu_change_t a;
    a.val = 0;
    return a;
}

void mapper_nsf::reset_map() {
    if(cart->uses_banks) {
        for(int i=0;i<8;++i)
            bank[i] = cart->get_header(0x70+i);
    }
    else {
        for(int i=0;i<8;++i)
            bank[i] = i;
    }
}
