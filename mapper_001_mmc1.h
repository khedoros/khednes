#pragma once
#include "rom.h"
#include "mapper.h"

//Implements Mapper #0 (aka "No mapper"), and provides the base class for other, more complex mappers.
class mapper_001: public mapper {
private:
    typedef enum {
        lo_crom,
        hi_crom,
        set_horiz,
        set_vert,
        set_4_scr,
        set_bg,
        set_spr
    } rom_command_t;
public:
    mapper_001(rom * r);
    virtual const unsigned int get_pbyte(const unsigned int addr);
    virtual const unsigned int get_pword(const unsigned int addr);
    virtual const unsigned int get_cbyte(const unsigned int addr);
    virtual const unsigned int get_page(const unsigned int addr);
    virtual void put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr);
    virtual bool put_cbyte(const unsigned int val,const unsigned int addr);
    virtual int changed_crom();
    virtual void ppu_change(unsigned int cycle, rom_command_t comm, unsigned int val);
    virtual rom::ppu_change_t cycle_forward(unsigned int cycle);
private:
    std::list<std::pair<unsigned int, std::pair<rom_command_t, unsigned int>>> ppu_queue;


    int tempval(bool * val);
    typedef struct {
        union { //$8000->$9FFF
            unsigned val:5;
            struct {
                unsigned mirror:1; //0=vert, 1=horiz
                unsigned obey_mirror:1; //0=one-screen, 1=use mirror mode
                unsigned high_low:1; //0=switch high PRG, 1=switch low PRG
                unsigned large_small_prg:1; //0=32KiB PRG switch, 1=16KiB PRG switch
                unsigned large_small_chr:1; //0=8KiB CHR switch, 1=4KiB CHR switch
            };
        } reg0;
        union { //$A000->$BFFF
            unsigned val:5;
            struct {
                unsigned low_chr_bank:4;
                unsigned low_big_prg:1;
            };
        } reg1;
        union { //$C000->$DFFF
            unsigned val:5;
            struct {
                unsigned high_chr_bank:4;
                unsigned high_big_prg:1;
            };
        } reg2;
        union { //$E000->$FFFF
            unsigned val:4;
            unsigned prg_bank:4;
        } reg3;
        unsigned int counter;
        bool temp[5];
    } mmc1;

    mmc1 mmc1_reg;
    unsigned int prg_lo_offset;
    unsigned int prg_hi_offset;
    unsigned int chr_lo_offset;
    unsigned int chr_hi_offset;
};
