#include "mapper.h"

mapper::mapper(rom* r) {
    cart = r;
}
const unsigned int mapper::get_pbyte(const unsigned int addr) {
    if(cart->prom_pages > 1) return cart->prom[addr-0x8000];
    else if(addr < 0xC000)
        return cart->prom[addr-0x8000];
    else
        return cart->prom[addr-0xC000];
}

const unsigned int mapper::get_pword(const unsigned int addr) {
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

const unsigned int mapper::get_cbyte(const unsigned int addr) {
    if(addr>=8192) {
        std::cout<<"Trying to get CHR byte at address: "<<addr<<std::endl;
        return 0;
    }
    return cart->crom[addr];
}

void mapper::put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr) {
}

bool mapper::put_cbyte(const unsigned int val,const unsigned int addr) {
    return false;
}

int mapper::changed_crom() {
    return 0;
}

void mapper::ppu_change(unsigned int cycle, unsigned int addr, unsigned int val) {
}

rom::ppu_change_t mapper::cycle_forward(unsigned int cycle) {
    rom::ppu_change_t a;
    a.val = 0;
    return a;
}

void mapper::reset_map() {}

const unsigned int mapper::get_page(const unsigned int addr) {
    if(addr < 0xc000) {
        return 0;
    }
    else {
        return 1;
    }
}
