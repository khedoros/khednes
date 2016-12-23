#include "mapper_nsf.h"

mapper_nsf::mapper_nsf(rom* r) : mapper(r) {
    cart = r;
}
const unsigned int mapper_nsf::get_pbyte(const unsigned int addr) {
    if(cart->prom_pages > 1) return cart->prom[addr-0x8000];
    else if(addr < 0xC000)
        return cart->prom[addr-0x8000];
    else
        return cart->prom[addr-0xC000];
}

const unsigned int mapper_nsf::get_pword(const unsigned int addr) {
    if(cart->prom_pages > 1) return cart->prom_w[addr-0x8000];
    else if(addr < 0xC000) {
        //std::cout<<"Reading: 0x"<<std::hex<<addr<<" (0x"<<(addr-0x8000+prg_lo_offset)<<")"<<std::endl;
        return cart->prom_w[addr-0x8000];
    }
    else {
        //std::cout<<"Reading: 0x"<<std::hex<<addr<<" (0x"<<(addr-0xc000+prg_hi_offset)<<")"<<std::endl;
        return cart->prom_w[addr-0xC000];
    }
}

const unsigned int mapper_nsf::get_cbyte(const unsigned int addr) {
    if(addr>=8192) {
        std::cout<<"Trying to get CHR byte at address: "<<addr<<std::endl;
        return 0;
    }
    return cart->crom[addr];
}

void mapper_nsf::put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr) {
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
    //Actually implement map reset in here
}
