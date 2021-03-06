#pragma once
#include "rom.h"
#include "mapper.h"

//Implements the NSF audio format mapper
class mapper_nsf : public mapper {
protected:
    typedef enum {
    } rom_command_t;
public:
    mapper_nsf(rom * r);
    virtual const unsigned int get_pbyte(const unsigned int addr);
    virtual const unsigned int get_pword(const unsigned int addr);
    virtual const unsigned int get_cbyte(const unsigned int addr);
    virtual void put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr);
    virtual bool put_cbyte(const unsigned int val,const unsigned int addr);
    virtual int changed_crom();
    virtual void ppu_change(unsigned int cycle, unsigned int addr, unsigned int val);
    virtual rom::ppu_change_t cycle_forward(unsigned int cycle);
    void reset_map();
protected:
    rom * cart;
    unsigned int bank[8];
};
