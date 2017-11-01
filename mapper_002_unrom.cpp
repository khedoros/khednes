#include "mapper_002_unrom.h"

mapper_002::mapper_002(rom* r): mapper(r) {
//    cart = r;
    prg_lo_offset = 0;
    prg_hi_offset = (r->prom_pages - 1) * PRG_PAGE_SIZE;
}

const unsigned int mapper_002::get_pbyte(const unsigned int addr) {
    if(addr < 0xC000)
        return cart->prom[addr-0x8000+prg_lo_offset];
    else
        return cart->prom[addr-0xC000+prg_hi_offset];
}

const unsigned int mapper_002::get_pword(const unsigned int addr) {
    if(addr < 0xC000) {
        //std::cout<<"Reading: 0x"<<std::hex<<addr<<" (0x"<<(addr-0x8000+prg_lo_offset)<<")"<<std::endl;
        return cart->prom_w[addr-0x8000+prg_lo_offset];
    }
    else {
        //std::cout<<"Reading: 0x"<<std::hex<<addr<<" (0x"<<(addr-0xc000+prg_hi_offset)<<")"<<std::endl;
        return cart->prom_w[addr-0xC000+prg_hi_offset];
    }
}

void mapper_002::put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr) {
    int real_val = val & get_pbyte(addr);
    //std::cout<<"Page from "<<std::hex<<(prg_lo_offset / PRG_PAGE_SIZE)<<" to "<<real_val<<std::endl;
    prg_lo_offset = PRG_PAGE_SIZE * real_val;
}

bool mapper_002::put_cbyte(const unsigned int val,const unsigned int addr) {
    if(cart->crom_pages == 0) {
        cart->crom[addr]=val;
        cart->updated_crom = 3;
        return true;
    }
    else return false;
}

int mapper_002::changed_crom() {
    int retval = cart->updated_crom;
    cart->updated_crom = 0;
    return retval;
}

const unsigned int mapper_002::get_page(const unsigned int addr) {
    if(addr < 0xc000) {
        return prg_lo_offset / PRG_PAGE_SIZE;
    }
    else {
        return prg_hi_offset / PRG_PAGE_SIZE;
    }
}

