#include "mapper_001_mmc1.h"

mapper_001::mapper_001(rom * r): mapper(r) {
    prg_lo_offset=0;
    prg_hi_offset=(cart->prom_pages - 1) * PRG_PAGE_SIZE;
    chr_lo_offset = 0;
    chr_hi_offset = 0x1000;

    mmc1_reg.reg0.val = 0x0c;
    mmc1_reg.reg1.val = 0;
    mmc1_reg.reg2.val = 0;
    mmc1_reg.reg3.val = 0;
    mmc1_reg.counter = 0;
    for(int i=0;i<5;++i) {
        mmc1_reg.temp[i]=false;
    }

    if(0x01 & (cart->header[6])) {
        mmc1_reg.reg0.mirror=false;
        mmc1_reg.reg0.obey_mirror=true;
    }
    else if(0x08 & (cart->header[6])) {
        mmc1_reg.reg0.mirror=false;
        mmc1_reg.reg0.obey_mirror=false;
    }
    else {
        mmc1_reg.reg0.mirror=true;
        mmc1_reg.reg0.obey_mirror=true;
    }
    mmc1_reg.reg0.high_low = true;
    mmc1_reg.reg0.large_small_prg = true;
    mmc1_reg.reg0.large_small_chr = false;
}

const unsigned int mapper_001::get_pbyte(const unsigned int addr) {
    if(addr < 0xC000) {
        //std::cout<<std::hex<<"Addr: "<<addr<<"("<<addr-0x8000+prg_lo_offset<<")"<<std::endl;
        return cart->prom[addr-0x8000 + prg_lo_offset];
    }
    else {
        //std::cout<<std::hex<<"Addr: "<<addr<<"("<<addr-0xc000+prg_hi_offset<<")"<<std::endl;
        //std::cout<<"Cart: "<<cart<<std::endl;
        return cart->prom[addr-0xC000 + prg_hi_offset];
    }
}

const unsigned int mapper_001::get_pword(const unsigned int addr) {
    if(addr < 0xC000) {
        //std::cout<<"Reading: 0x"<<std::hex<<addr<<" (0x"<<(addr-0x8000+prg_lo_offset)<<")"<<std::endl;
        return cart->prom_w[addr-0x8000 + prg_lo_offset];
    }
    else {
        //std::cout<<"Reading: 0x"<<std::hex<<addr<<" (0x"<<(addr-0xc000+prg_hi_offset)<<")"<<std::endl;
        return cart->prom_w[addr-0xC000 + prg_hi_offset];
    }
}

const unsigned int mapper_001::get_cbyte(const unsigned int addr) {
    if(addr>=8192) {
        std::cout<<"Trying to get CHR byte at address: "<<addr<<std::endl;
        return 0;
    }
    if(addr < 0x1000) return cart->crom[addr+chr_lo_offset];
    else return cart->crom[addr-0x1000+chr_hi_offset];
}

void mapper_001::put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr) {
    //std::printf("MMC1: %04x = %02x (",addr,val);
    if(val < 128 && mmc1_reg.counter < 4) {
        if(mmc1_reg.counter == 0) {
            //std::cout<<std::endl<<std::endl<<"Initializing new MMC1 register write!"<<std::endl;
        }
        //std::printf("Set temp[%d] to %d\n",mmc1_reg_counter,((val&1)==1?1:0));
        if((val & 1) == 1) {
            mmc1_reg.temp[mmc1_reg.counter]=true;
        }
        else {
            mmc1_reg.temp[mmc1_reg.counter]=false;
        }
        ++(mmc1_reg.counter);
    } else if(val < 128 && mmc1_reg.counter == 4) {
        if(val & 1) {
            mmc1_reg.temp[mmc1_reg.counter]=true;
        }
        else {
            mmc1_reg.temp[mmc1_reg.counter]=false;
        }
        mmc1_reg.counter=0;
        //std::printf("MMC1: %04x = %02x\n",addr,tempval(mmc1_reg.temp));
        //std::cout<<"Setting addr "<<std::hex<<addr<<" to "<<tempval(mmc1_reg.temp)<<std::endl;
        if(addr >= 0x8000 && addr <= 0x9fff) {
            int mirror = mmc1_reg.reg0.mirror;
            int one_screen = mmc1_reg.reg0.obey_mirror;
            mmc1_reg.reg0.val = tempval(mmc1_reg.temp);
            if(mirror != mmc1_reg.reg0.mirror) {
                std::cout<<"Mirror mode changed to "<<!mirror<<"!"<<std::endl;
                if(mmc1_reg.reg0.mirror == 0) ppu_change(cycle, set_vert, 0);
                if(mmc1_reg.reg0.mirror == 1) ppu_change(cycle, set_horiz, 0);
            }
            if(one_screen != mmc1_reg.reg0.obey_mirror) {
                std::cout<<"One-screen mode changed!"<<std::endl;
                if(mmc1_reg.reg0.obey_mirror == 0) ppu_change(cycle, set_4_scr, 0);
                else if(mmc1_reg.reg0.mirror == 0) ppu_change(cycle, set_vert, 0);
                else if(mmc1_reg.reg0.mirror == 1) ppu_change(cycle, set_horiz, 0);
            }
            //std::cout<<"Mirror: "<<(mmc1_reg.reg0.mirror?'H':'V')<<" One-Screen: "<<(!mmc1_reg.reg0.obey_mirror);
            //std::cout<<" PRG Switch: "<<(mmc1_reg.reg0.high_low?'L':'H')<<" PRG Size: "<<(mmc1_reg.reg0.large_small_prg?'S':'L');
            //std::cout<<" CHR Size: "<<(mmc1_reg.reg0.large_small_chr?'S':'L')<<std::endl;
        } else if(addr >= 0xa000 && addr <= 0xbfff) {
            mmc1_reg.reg1.val = tempval(mmc1_reg.temp);
            if(mmc1_reg.reg0.large_small_chr) { //small chr
                cart->updated_crom = 3;
                int new_chr_lo_offset = mmc1_reg.reg1.val * 0x1000;
                int new_chr_hi_offset = mmc1_reg.reg2.val * 0x1000;
                ppu_change(cycle, lo_crom, new_chr_lo_offset);
                ppu_change(cycle, hi_crom, new_chr_hi_offset);
            } else {
                cart->updated_crom = 3;
                int new_chr_lo_offset = mmc1_reg.reg1.val * 0x2000;
                int new_chr_hi_offset = mmc1_reg.reg1.val * 0x2000 + 0x1000;
                ppu_change(cycle, lo_crom, new_chr_lo_offset);
                ppu_change(cycle, hi_crom, new_chr_hi_offset);
            }
            //std::cout<<"CHR ROM bank switching not properly implemented. Sry, dude!"<<std::endl;
        } else if(addr >= 0xc000 && addr <= 0xdfff) {
            mmc1_reg.reg2.val=tempval(mmc1_reg.temp);
            if(mmc1_reg.reg0.large_small_chr) { //small chr
                cart->updated_crom = 3;
                int new_chr_lo_offset = mmc1_reg.reg1.val * 0x1000;
                int new_chr_hi_offset = mmc1_reg.reg2.val * 0x1000;
                ppu_change(cycle, lo_crom, new_chr_lo_offset);
                ppu_change(cycle, hi_crom, new_chr_hi_offset);
            } else {
                cart->updated_crom = 3;
                int new_chr_lo_offset = mmc1_reg.reg1.val * 0x2000;
                int new_chr_hi_offset = mmc1_reg.reg1.val * 0x2000 + 0x1000;
                ppu_change(cycle, lo_crom, new_chr_lo_offset);
                ppu_change(cycle, hi_crom, new_chr_hi_offset);
            }
            //std::cout<<"CHR ROM bank switching not properly implemented. Sry, dude!"<<std::endl;
        } else if(addr >= 0xe000 && addr <= 0xffff) {
            mmc1_reg.reg3.val=tempval(mmc1_reg.temp);
            if(!mmc1_reg.reg0.large_small_prg) { //32KiB switching is "false" value
                int temp = mmc1_reg.reg3.val >>(1);
                prg_lo_offset=temp*0x8000;
                prg_hi_offset=temp*0x8000+PRG_PAGE_SIZE;
            } else if(mmc1_reg.reg0.high_low) { //Switch low PRG
                prg_lo_offset=mmc1_reg.reg3.val*16384;
                prg_hi_offset=(cart->prom_pages -1) * PRG_PAGE_SIZE;
            } else { //Switch high PRG
                prg_lo_offset=0;
                prg_hi_offset=mmc1_reg.reg3.val*16384;
            }
        }
    } else if(val>=128) {
        //std::printf("MMC1: %04x reset\n",addr);
        mmc1_reg.counter=0;
        if(addr>=0x8000&&addr<=0x9fff) {
            //mmc1_reg.reg0.val = 0;
            mmc1_reg.reg0.val |= 0x0C;
            //mmc1_reg.reg0.high_low = 1;
            //mmc1_reg.reg0.large_small_prg = 1;
            //std::cout<<"Clear reg 0"<<std::endl;
        } else if(addr>=0xa000&&addr<=0xbfff) {
            mmc1_reg.reg1.val = 0;
            //std::cout<<"Clear reg 1"<<std::endl;
        } else if(addr>=0xc000&&addr<=0xdfff) {
            mmc1_reg.reg2.val = 0;
            //std::cout<<"Clear reg 2"<<std::endl;
        } else if(addr>=0xe000&&addr<=0xffff) {
            mmc1_reg.reg3.val = 0;
            //std::cout<<"Clear reg 3"<<std::endl;
        }
    }
}

bool mapper_001::put_cbyte(const unsigned int val,const unsigned int addr) {
    if(cart->crom_pages == 0) {
        cart->crom[addr]=val;
        return true;
    }
    else return false;
}

int mapper_001::changed_crom() {
    return 0;
}

void mapper_001::ppu_change(unsigned int cycle, rom_command_t comm, unsigned int val) {
    ppu_queue.push_back(std::make_pair(cycle, std::make_pair(comm, val)));
}

rom::ppu_change_t mapper_001::cycle_forward(unsigned int cycle) {
    rom::ppu_change_t p;
    p.val=0;
    return p;
}

int mapper_001::tempval(bool * val) {
    int retval = 0;
    for(int i = 4; i >= 0; --i) {
        retval<<=(1);
        retval |= int(val[i]);
    }
    return retval;
}

const unsigned int mapper_001::get_page(const unsigned int addr) {
    if(addr < 0xc000) {
        return prg_lo_offset / PRG_PAGE_SIZE;
    }   
    else {
        return prg_hi_offset / PRG_PAGE_SIZE;
    }   
}
