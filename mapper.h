#pragma once
#include "rom.h"

//Implements Mapper #0 (aka "No mapper"), and provides the base class for other, more complex mappers.
class mapper {
protected:
    typedef enum {
    } rom_command_t;
public:
    mapper(rom * r);
    virtual const unsigned int get_pbyte(const unsigned int addr);
    virtual const unsigned int get_pword(const unsigned int addr);
    virtual const unsigned int get_cbyte(const unsigned int addr);
    virtual const unsigned int get_page(const unsigned int addr);
    virtual void put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr);
    virtual bool put_cbyte(const unsigned int val,const unsigned int addr);
    virtual int changed_crom();
    virtual void ppu_change(unsigned int cycle, unsigned int addr, unsigned int val);
    virtual rom::ppu_change_t cycle_forward(unsigned int cycle);
    virtual void reset_map();
protected:
    rom * cart;
};
