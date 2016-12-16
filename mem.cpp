#include<iostream>
#include<fstream>
#include "mem.h"
#define ORIGIN 3
using namespace std;

mem::mem(rom &romi, ppu &ppui,apu &apui) : cart(romi), pu(ppui), snd(apui), cycle(0), mouse_x(0), mouse_y(0) {
    ram.resize(0x800);
    sram.resize(0x2000);
    for(int i=0;i<8;i++) {
        joy1_buttons[i]=false;
    }
    if(cart.has_sram()) {
        return;
        string infile = cart.filename();
        infile += ".sav";
        cout<<"Trying to open "<<infile<<endl;
        ifstream inf;
        inf.open(infile.c_str());
        if(inf.is_open()) {
            inf.seekg(0,ios::end);
            size_t filesize = inf.tellg();
            inf.seekg(0,ios::beg);
            if(filesize == 0x2000) {
                inf.read(reinterpret_cast<char *>(&(sram[0])), 0x2000);
                cout<<"Loaded "<<infile<<" as save game."<<endl;
            }
            inf.close();
        }
        else cout<<"Couldn't open "<<infile<<" for input."<<endl;
    }
}

mem::~mem() {
    if(cart.has_sram()) {
        return;
        string outfile = cart.filename();
        outfile += ".sav";
        cout<<"Saving to "<<outfile<<endl;
        ofstream sram_out;
        sram_out.open(outfile.c_str(), ios::out|ios::trunc);
        if(sram_out.is_open()) {
            sram_out.write(reinterpret_cast<char *>(&(sram[0])), 0x2000);
            cout<<"Saved file "<<outfile<<endl;
        }
    }
}

void mem::sendkeydown(SDL_Scancode test) {
        //util::debug(ORIGIN,"Key down! (%04x)\n",test);
        switch(test) {
                case SDL_SCANCODE_A:
                        joy1_buttons[6]=true; //left
                        //printf("LEFT!\n");
                        break;
                case SDL_SCANCODE_D:
                        joy1_buttons[7]=true; //right
                        //printf("RIGHT! ");
                        break;
                case SDL_SCANCODE_G:
                        joy1_buttons[2]=true; //select
                        //printf("SELECT!\n");
                        break;
                case SDL_SCANCODE_H:
                        joy1_buttons[3]=true; //start
                        //printf("START!\n");
                        break;
                case SDL_SCANCODE_K:
                        joy1_buttons[1]=true; //b
                        //printf("B!\n");
                        break;
                case SDL_SCANCODE_L:
                        joy1_buttons[0]=true; //a
                        //printf("A!\n");
                        break;
                case SDL_SCANCODE_S:
                        joy1_buttons[5]=true; //down
                        //printf("DOWN!\n");
                        break;
                case SDL_SCANCODE_W:
                        joy1_buttons[4]=true; //up
                        //printf("UP!\n");
                        break;
                case SDL_SCANCODE_GRAVE:
                        pu.speedup(true);
                        break;
                case SDL_SCANCODE_X:
                        joy2_trigger=16;
                        break;
                case SDL_SCANCODE_Z:
                        joy2_light=8;
                        break;
                default: cout<<"Saw button "<<int(test)<<endl;
        }        
}

void mem::sendkeyup(SDL_Scancode test) {
        //util::debug(ORIGIN,"Key up! (%04x)\n",test);
        switch(test) {
                case SDL_SCANCODE_A:
                        joy1_buttons[6]=false; //left
                        break;
                case SDL_SCANCODE_D:
                        joy1_buttons[7]=false; //right
                        break;
                case SDL_SCANCODE_G:
                        joy1_buttons[2]=false; //select
                        break;
                case SDL_SCANCODE_H:
                        joy1_buttons[3]=false; //start
                        break;
                case SDL_SCANCODE_K:
                        joy1_buttons[1]=false; //b
                        break;
                case SDL_SCANCODE_L:
                        joy1_buttons[0]=false; //a
                        break;
                case SDL_SCANCODE_S:
                        joy1_buttons[5]=false; //down
                        break;
                case SDL_SCANCODE_W:
                        joy1_buttons[4]=false; //up
                        break;
                case SDL_SCANCODE_GRAVE:
                        pu.speedup(false);
                        break;
                case SDL_SCANCODE_X:
                        joy2_trigger=0;
                        break;
                case SDL_SCANCODE_Z:
                        joy2_light=0;
                        break;
                default: //don't really care about the rest of the values =)
                        break;
        }
}

void mem::sendmousepos(int x, int y) {
    mouse_x = x;
    mouse_y = y;
}

void mem::sendmouseup(int x, int y) {
    //cout<<"Mouse up at "<<dec<<x<<", "<<y<<endl;
    joy2_trigger = 0;
}

void mem::sendmousedown(int x,int y) {
    //cout<<"Mouse down at "<<dec<<x<<", "<<y<<endl;
    //cout<<"Color: "<<hex<<pu.get_buffer_color(x,y)<<endl;
    joy2_trigger = 16;
}

const unsigned int mem::read(unsigned int address) {
        address&=0xFFFF;
        if(address<=0x1FFF) { 
                return ram[address&0x7ff];
        }
        if(address>=0x8000) {
                return cart.get_pbyte(address);
        }
        if(address>=0x2000&&address<0x4000) {
                //util::debug(ORIGIN, "Reading ppu register address %04x\n",address);
                return pu.reg_read((address&7)+0x2000, cycle);
        }
        if((address>=0x4000&&address<0x4014)||address==0x4015) {
                //util::debug(ORIGIN, "TODO: unimplemented audio stuff.\n");
                //if(address == 0x4015) cout<<"Read sound status"<<endl;
        return snd.read_status();
                //return 0;
        }
        if(address==0x4016) {//||address==0x4017) {
                //util::debug(ORIGIN, "TODO: unimplemented joypad handling.\n");
                if(joy1_strobe) {
                        //util::debug(ORIGIN,"A (strobe is on)\n");
                        //printf("Strobe is on!\n");
                        return joy1_buttons[0];
                }
                else if(joy1_bit<8) {
                        int retval=(joy1_buttons[joy1_bit])?1:0;
                        joy1_bit++;
                        return retval;
                }
                else {
                        return 1;
                }
                return 0;
        }
        else if(address==0x4017) {
                uint32_t color = pu.get_buffer_color(mouse_x, mouse_y);
                color |= pu.get_buffer_color(mouse_x+5, mouse_y+5);
                color |= pu.get_buffer_color(mouse_x+5, mouse_y-5);
                color |= pu.get_buffer_color(mouse_x-5, mouse_y+5);
                color |= pu.get_buffer_color(mouse_x-5, mouse_y-5);
                uint8_t r = color>>(16);
                uint8_t g = (color&0xFF00)>>(8);
                uint8_t b = (color&0xFF);
                if(r > 0x80 || g > 0x80 || b > 0x80) joy2_light = 8;
                else                                 joy2_light = 0;
                //cout<<"joy2_light: "<<((joy2_light)?"yes":"no")<<endl;
                if(joy1_strobe) {
                    return joy2_buttons[0];
                }
                else if(joy2_bit<8) {
                    int retval=(joy2_buttons[joy2_bit])?1:0;
                    joy2_bit++;
                    return retval|joy2_trigger|joy2_light;
                }
                else {
                    return 1|joy2_trigger|joy2_light;
                }
                return 0;
        }
        else if(address>=0x6000&&address<=0x7fff) {
            return sram[address&0x1fff];
        }
        util::debug(ORIGIN, "mem read of %04x unhandled. I don't know what the address does.\n",address);
        return 0;
}


const unsigned int mem::read2(unsigned int address) {        
        address&=0xFFFF;
        if(address<0x1FFF) { //0x1FFF because I'm returning 2 bytes
                unsigned int temp1=ram[(address+1)&0x7FF];
                temp1<<=(8);
                temp1|=ram[address&0x7FF];
                return temp1;
        }
        if(address>=0x6000&&address<=0x7fff) {
                unsigned int temp1=sram[(address+1)&0x1fff];
                temp1<<=(8);
                temp1|=sram[address&0x1fff];
                return temp1;
        }
        if(address>=0x8000) {
                //unsigned int temp1=cart.get_pbyte(address+1);
                //temp1<<=(8);
                //temp1|=cart.get_pbyte(address);
                //return temp1;
                return cart.get_pword(address);
        }
        util::debug(ORIGIN, "mem read(2) of %04x unhandled. I don't know what the address does.\n",address);
        return 0;
}

void mem::write(unsigned int address, unsigned char val) {
        address&=0xFFFF;
        if(address>=0x2000&&address<0x4000) {
                //cout<<"Writing to PPU "<<endl;
                pu.reg_write((address&7)+0x2000,val,cycle);
        }
        else if(address<=0x1FFF) {
                ram[address&0x7FF]=val;
        }
        else if(address==0x4014) {
                for(int ii=0;ii<256;ii++) {
                        pu.dma(this->read(((int(val))<<(8))+ii),cycle);
                }
        }
        else if((address>=0x4000&&address<0x4014)||address==0x4015||address==0x4017) {
                //util::debug(ORIGIN,"TODO: unimplemented audio stuff.\n");
                snd.reg_write(frame, cycle, address,val);
        }
        else if(address==0x4016) {
                //util::debug(ORIGIN,"TODO: unimplemented joypad handling.\n");
                if((val&0x01)==1) {
                                        joy1_strobe=true;
                                        joy1_bit=0;
                                }
                                else {
                                        joy1_strobe=false;
                                }
        }
        else if(address>=0x6000&&address<=0x7fff) {
            //std::cout<<"Writing SRAM address "<<std::hex<<(address&0x1fff)<<std::endl;
            sram[address&0x1fff]=val;
        }
        else if(address>=0x8000&&address<=0xffff) {
            if(pu.phase(cycle) == ppu::RENDER) {
                //cout<<dec<<cycle<<": 0x"<<hex<<address<<" = "<<int(val)<<dec<<endl;
            }
            cart.put_pbyte(cycle, val,address);
        }
        else {
                printf("Hey! Don't try to write %02x to %04x!!\n",val,address);
                util::debug(ORIGIN,"I don't know what writing %02x to %04x does.\n",val,address);
        }
}

const unsigned int mem::get_rst_addr() {
        return cart.get_rst_addr();
}

const unsigned int mem::get_nmi_addr() {
        return cart.get_nmi_addr();
}

const unsigned int mem::get_irq_addr() {
        return cart.get_irq_addr();
}

const unsigned int mem::get_ppu_cycles(const unsigned int frame) {
        return pu.get_ppu_cycles(frame);
}

void mem::set_cycle(unsigned int cycle) {
    this->cycle = cycle;
}

void mem::set_frame(unsigned int frame) {
    this->frame = frame;
}
