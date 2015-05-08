#include "mapper.h"
#include "mapper_003_cnrom.h"

mapper_003::mapper_003(rom* r) : mapper(r) {
    chr_lo_offset = 0;
    chr_hi_offset = 0x1000;
}
const unsigned int mapper_003::get_pbyte(const unsigned int addr) {
    if(cart->prom_pages > 1) return cart->prom[addr-0x8000];
    else if(addr < 0xC000)
        return cart->prom[addr-0x8000];
    else
        return cart->prom[addr-0xC000];
}

const unsigned int mapper_003::get_pword(const unsigned int addr) {
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

const unsigned int mapper_003::get_cbyte(const unsigned int addr) {
    if(addr>=8192) {
        std::cout<<"Trying to get CHR byte at address: "<<addr<<std::endl;
        return 0;
    }
    return cart->crom[addr];
}

void mapper_003::put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr) {
        cart->updated_crom = 3;
        int real_val = ((val & get_pbyte(addr))&0x3);
        int new_chr_lo_offset = CHR_PAGE_SIZE * real_val;
        int new_chr_hi_offset = new_chr_lo_offset + 0x1000;
        ppu_change(cycle, lo_crom, new_chr_lo_offset);
        ppu_change(cycle, hi_crom, new_chr_hi_offset);
}

bool mapper_003::put_cbyte(const unsigned int val,const unsigned int addr) {
    return false;
}

int mapper_003::changed_crom() {
    int retval = cart->updated_crom;
    cart->updated_crom = 0;
    return retval;
}

void mapper_003::ppu_change(unsigned int cycle, rom_command_t comm, unsigned int val) {
    ppu_queue.push_back(std::make_pair(cycle, std::make_pair(comm, val)));
}

rom::ppu_change_t mapper_003::cycle_forward(unsigned int cycle) {
    unsigned int next_c = cycle + 1;
    if(ppu_queue.size() > 0) {
        next_c = ppu_queue.front().first;
    }
    rom::ppu_change_t changes;
    changes.val = 0;
    while(ppu_queue.size() > 0 && next_c < cycle) {
        rom_command_t comm = ppu_queue.front().second.first;
        int val = ppu_queue.front().second.second;
        switch(comm) {
        case lo_crom:
            changes.lo_crom = 1;
            chr_lo_offset = val;
            break;
        case hi_crom:
            changes.hi_crom = 1;
            chr_hi_offset = val;
            break;
        case set_horiz:
            changes.set_horiz = 1;
            cart->mirror_mode = HORIZ;
            break;
        case set_vert:
            changes.set_vert = 1;
            cart->mirror_mode = VERT;
            break;
        case set_4_scr:
            changes.set_4_scr = 1;
            cart->mirror_mode = FOUR_SCR;
            break;
        case set_bg:
            if(val == 0)
                changes.set_bg0 = 1;
            else
                changes.set_bg1 = 1;
            break;
        case set_spr:
            if(val == 0)
                changes.set_spr0 = 1;
            else
                changes.set_spr1 = 1;
            break;
        default:
            std::cout<<"Unknown command!"<<std::endl;
            break;
        }
        ppu_queue.pop_front();
        if(ppu_queue.size() > 0)
            next_c = ppu_queue.front().first;
    }
    return changes;

}
