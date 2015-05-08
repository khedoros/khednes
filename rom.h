#ifndef ROM_H
#define ROM_H
#include<fstream>
#include<iostream>
#include<list>
#include "nes_constants.h"
#include "util.h"

class mapper;
class mapper_001;
class mapper_002;
class mapper_003;
class mapper_011;

class rom {
friend mapper;
friend mapper_001;
friend mapper_002;
friend mapper_003;
friend mapper_011;

public:
        rom(std::string filename, int mapper);
        ~rom();
        bool isValid();
        void print_info();
        const unsigned int get_rst_addr();
        const unsigned int get_nmi_addr();
        const unsigned int get_irq_addr();
        const unsigned int get_pbyte(const unsigned int addr);
        const unsigned int get_pword(unsigned int);
        const unsigned int get_cbyte(const unsigned int addr);
        const unsigned int mmc1_readp(const unsigned int addr);
        bool put_cbyte(const unsigned int val,const unsigned int addr);
        void put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr);
        const unsigned int get_mode();
        int changed_crom();
        bool has_sram();
        const std::string& filename();

        void ppu_change(unsigned int cycle, unsigned int addr, unsigned int val); //To cue up mid-frame PPU changes this frame

        typedef union {
            unsigned val:9;
            struct {
                unsigned lo_crom:1;
                unsigned hi_crom:1;
                unsigned set_horiz:1;
                unsigned set_vert:1;
                unsigned set_4_scr:1;
                unsigned set_bg0:1;
                unsigned set_bg1:1;
                unsigned set_spr0:1;
                unsigned set_spr1:1;
            };
        } ppu_change_t;

        ppu_change_t cycle_forward(unsigned int cycle); //To pop cued changes up to the given frame time


private:

        bool load(std::string& filename);
        int mapper_guess();
        mapper *map;

        //cycle, (command, value)
        bool valid;
        //void update_pattern(int tile,int bank);
        Vect<unsigned char> header;
        Vect<unsigned char> prom;
        Vect<unsigned int> prom_w;
        Vect<unsigned char> crom;
        const std::string & file;
        int prom_pages;
        int crom_pages;
        int ppage;
        int cpage;
        int mirror_mode;
        int filesize;
        int mapper_num;
        int rst_addr;
        int nmi_addr;
        int irq_addr;
        bool sram;
        bool trainer;
        int updated_crom;
        unsigned int prg_lo_offset;
        unsigned int prg_hi_offset;
        unsigned int chr_lo_offset;
        unsigned int chr_hi_offset;
};
#endif
