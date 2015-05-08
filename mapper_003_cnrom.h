#ifndef MAPPER_003_H
#define MAPPER_003_H
#include "rom.h"

//Implements Mapper #0 (aka "No mapper"), and provides the base class for other, more complex mappers.
class mapper_003: public mapper {
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
    mapper_003(rom * r);
    virtual const unsigned int get_pbyte(const unsigned int addr);
    virtual const unsigned int get_pword(const unsigned int addr);
    virtual const unsigned int get_cbyte(const unsigned int addr);
    virtual void put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr);
    virtual bool put_cbyte(const unsigned int val,const unsigned int addr);
    virtual int changed_crom();
    virtual void ppu_change(unsigned int cycle, rom_command_t comm, unsigned int val);
    virtual rom::ppu_change_t cycle_forward(unsigned int cycle);

private:
    std::list<std::pair<unsigned int, std::pair<rom_command_t, unsigned int>>> ppu_queue;
    unsigned int chr_lo_offset;
    unsigned int chr_hi_offset;
};
#endif
