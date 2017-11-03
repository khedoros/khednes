#pragma once
#include "rom.h"
#include "mapper.h"

//Implements Mapper #0 (aka "No mapper"), and provides the base class for other, more complex mappers.
class mapper_002: public mapper {
public:
    mapper_002(rom * r);
    virtual const unsigned int get_pbyte(const unsigned int addr);
    virtual const unsigned int get_pword(const unsigned int addr);
    virtual const unsigned int get_page(const unsigned int addr);
    virtual void put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr);
    virtual bool put_cbyte(const unsigned int val,const unsigned int addr);
    virtual int changed_crom();
private:
    unsigned int prg_lo_offset;
    unsigned int prg_hi_offset;
};
