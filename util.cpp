#include<stdio.h>
#include<stdint.h>
#include "util.h"
#include<sstream>
#include<iomanip>
#define ORIGIN 6

bool util::init=false;
std::bitset<1000> util::active;

void util::toggle_log(int origin) {
    if(!init) {
        for(int i=0;i<1000;++i) active[i] = false;
        init=true;
    }
    active[origin]=!(active[origin]);
    switch(origin) {
    case 1:
        printf("1 (CPU)");
        break;
    case 2:
        printf("2 (MAIN)");
        break;
    case 3:
        printf("3 (MEM)");
        break;
    case 4:
        printf("4 (PPU)");
        break;
    case 5:
        printf("5 (ROM)");
        break;
    case 6:
        printf("6 (UTIL)");
        break;
    case 7:
        printf("7 (WAV)");
        break;
    case 8:
        printf("8 (APU)");
        break;
    }
}

bool util::is_active(int i) {
    return active[i];
}

int util::debug(int orig,const char * fmt,...) {
    if(!init) {
        for(int i=0;i<1000;++i) {
            active[i]=false;
        }
        init=true;
    }

    if(active[orig]||(orig==999&&active[1])) {
        switch(orig) {
                case 1:
                        printf("CPU: ");
                        break;
                case 2:
                        printf("MAIN: ");
                        break;
                case 3:
                        printf("MEM: ");
                        break;
                case 4:
                        printf("PPU: ");
                        break;
                case 5:
                        printf("ROM: ");
                        break;
                case 6:
                        printf("UTIL: ");
                        break;
                case 7:
                        printf("WAV: ");
                        break;
                case 8:
                        printf("APU: ");
                        break;
                default:
                        break; //Don't print origin label
        }
        va_list ap;
        va_start(ap,fmt);
        return vprintf(fmt,ap);
    }
    else {
        return 0;
    }
}

int util::debug(const char * fmt,...) {
    va_list ap;
    va_start(ap,fmt);
    return vprintf(fmt,ap);
}

std::string util::inst_string(int byte0, int byte1, int byte2) {
    std::string inst = "";
    if(byte0 >= 0 && byte0 < 256) {
        inst = inst_names[byte0];
    }
    else {
        return std::string("Opcode out of range");
    }
    std::stringstream ss;
    ss<<std::hex;
    switch(inst_types[byte0]) {
        case inv: //invalid operation
            return std::string("Unofficial/invalid opcode");
        //Fall-through for types that don't show any arguments
        case acc: //accumulator
        case imp: //implied
            break;
        case imm: //immediate
            ss<<" #$"<<std::setfill('0')<<std::setw(2)<<byte1;
            break;
        case zpa: //zero-page absolute
            ss<<" $"<<std::setfill('0')<<std::setw(2)<<byte1;
            break;
        case rel: //relative
            ss<<" $"<<std::setfill('0')<<std::setw(2)<<byte1;
            break;
        case abso: //absolute
            ss<<" $"<<std::setfill('0')<<std::setw(2)<<byte2<<std::setfill('0')<<std::setw(2)<<byte1;
            break;
        case absx://absolute, x-indexed
            ss<<" $"<<std::setfill('0')<<std::setw(2)<<byte2<<std::setfill('0')<<std::setw(2)<<byte1<<", X";
            break;
        case absy://absolute, y-indexed
            ss<<" $"<<std::setfill('0')<<std::setw(2)<<byte2<<std::setfill('0')<<std::setw(2)<<byte1<<", Y";
            break;
        case zpx: //zero-page absolute, x-indexed
            ss<<" $"<<std::setfill('0')<<std::setw(2)<<byte1<<", X";
            break;
        case zpy: //zero-page absolute, y-indexed
            ss<<" $"<<std::setfill('0')<<std::setw(2)<<byte1<<", Y";
            break;
        case ind: //indirect
            ss<<" ($"<<std::setfill('0')<<std::setw(2)<<byte2<<std::setfill('0')<<std::setw(2)<<byte1<<")";
            break;
        case zpix://zero-page indirect, x-pre-indexed
            ss<<" ($"<<std::setfill('0')<<std::setw(2)<<byte1<<",X)";
            break;
        case zpiy: //zero-page indirect, y-post-indexed
            ss<<" ($"<<std::setfill('0')<<std::setw(2)<<byte1<<"), Y";
            break;
        default:
            break;
            return std::string("Don't know how you got here.");
    }
    return inst + ss.str();
}

const std::string util::inst_names[] {
//       00     01     02     03     04     05     06     07     08     09     0A     0B     0C     0D     0E     0F
/*00*/  "BRK", "ORA", "   ", "   ", "   ", "ORA", "ASL", "   ", "PHP", "ORA", "ASL", "   ", "   ", "ORA", "ASL", "   ",
/*10*/  "BPL", "ORA", "   ", "   ", "   ", "ORA", "ASL", "   ", "CLC", "ORA", "   ", "   ", "   ", "ORA", "ASL", "   ",
/*20*/  "JSR", "AND", "   ", "   ", "BIT", "AND", "ROL", "   ", "PLP", "AND", "ROL", "   ", "BIT", "AND", "ROL", "   ",
/*30*/  "BMI", "AND", "   ", "   ", "   ", "AND", "ROL", "   ", "SEC", "AND", "   ", "   ", "   ", "AND", "ROL", "   ",
/*40*/  "RTI", "EOR", "   ", "   ", "   ", "EOR", "LSR", "   ", "PHA", "EOR", "LSR", "   ", "JMP", "EOR", "LSR", "   ",
/*50*/  "BVC", "EOR", "   ", "   ", "   ", "EOR", "LSR", "   ", "CLI", "EOR", "   ", "   ", "   ", "EOR", "LSR", "   ",
/*60*/  "RTS", "ADC", "   ", "   ", "   ", "ADC", "ROR", "   ", "PLA", "ADC", "ROR", "   ", "JMP", "ADC", "ROR", "   ",
/*70*/  "BVS", "ADC", "   ", "   ", "   ", "ADC", "ROR", "   ", "SEI", "ADC", "   ", "   ", "   ", "ADC", "ROR", "   ",
/*80*/  "   ", "STA", "   ", "   ", "STY", "STA", "STX", "   ", "DEY", "BIT", "TXA", "   ", "STY", "STA", "STX", "   ",
/*90*/  "BCC", "STA", "   ", "   ", "STY", "STA", "STX", "   ", "TYA", "STA", "TXS", "   ", "   ", "STA", "   ", "   ",
/*A0*/  "LDY", "LDA", "LDX", "   ", "LDY", "LDA", "LDX", "   ", "TAY", "LDA", "TAX", "   ", "LDY", "LDA", "LDX", "   ",
/*B0*/  "BCS", "LDA", "   ", "   ", "LDY", "LDA", "LDX", "   ", "CLV", "LDA", "TSX", "   ", "LDY", "LDA", "LDX", "   ",
/*C0*/  "CPY", "CMP", "   ", "   ", "CPY", "CMP", "DEC", "   ", "INY", "CMP", "DEX", "   ", "CPY", "CMP", "DEC", "   ",
/*D0*/  "BNE", "CMP", "   ", "   ", "   ", "CMP", "DEC", "   ", "CLD", "CMP", "   ", "   ", "   ", "CMP", "DEC", "   ",
/*E0*/  "CPX", "SBC", "   ", "   ", "CPX", "SBC", "INC", "   ", "INX", "SBC", "NOP", "   ", "CPX", "SBC", "INC", "   ",
/*F0*/  "BEQ", "SBC", "   ", "   ", "   ", "SBC", "INC", "   ", "SED", "SBC", "   ", "   ", "   ", "SBC", "INC", "   "
    };

const addr_mode util::inst_types[] {
//       00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F
/*00*/   imp, zpix,  inv,  inv,  inv,  zpa,  zpa,  inv,  imp,  imm,  acc,  inv,  inv, abso, abso,  inv,
/*10*/   rel, zpiy,  inv,  inv,  inv,  zpx,  zpx,  inv,  imp, absy,  inv,  inv,  inv, absx, absx,  inv,
/*20*/  abso, zpix,  inv,  inv,  zpa,  zpa,  zpa,  inv,  imp,  imm,  acc,  inv, abso, abso, abso,  inv,
/*30*/   rel, zpiy,  inv,  inv,  inv,  zpx,  zpx,  inv,  imp, absy,  inv,  inv,  inv, absx, absx,  inv,
/*40*/   imp, zpix,  inv,  inv,  inv,  zpa,  zpa,  inv,  imp,  imm,  acc,  inv, abso, abso, abso,  inv,
/*50*/   rel, zpiy,  inv,  inv,  inv,  zpx,  zpx,  inv,  imp, absy,  inv,  inv,  inv, absx, absx,  inv,
/*60*/   imp, zpix,  inv,  inv,  inv,  zpa,  zpa,  inv,  imp,  imm,  acc,  inv,  ind, abso, abso,  inv,
/*70*/   rel, zpiy,  inv,  inv,  inv,  zpx,  zpx,  inv,  imp, absy,  inv,  inv,  inv, absx, absx,  inv,
/*80*/   inv, zpix,  inv,  inv,  zpa,  zpa,  zpa,  inv,  imp,  imm,  imp,  inv, abso, abso, abso,  inv,
/*90*/   rel, zpiy,  inv,  inv,  zpx,  zpx,  zpy,  inv,  imp, absy,  imp,  inv,  inv, absx,  inv,  inv,
/*A0*/   imm, zpix,  imm,  inv,  zpa,  zpa,  zpa,  inv,  imp,  imm,  imp,  inv, abso, abso, abso,  inv,
/*B0*/   rel, zpiy,  inv,  inv,  zpx,  zpx,  zpy,  inv,  imp, absy,  imp,  inv, absx, absx, absy,  inv,
/*C0*/   imm, zpix,  inv,  inv,  zpa,  zpa,  zpa,  inv,  imp,  imm,  imp,  inv, abso, abso, abso,  inv,
/*D0*/   rel, zpiy,  inv,  inv,  inv,  zpx,  zpx,  inv,  imp, absy,  inv,  inv,  inv, absx, absx,  inv,
/*E0*/   imm, zpix,  inv,  inv,  zpa,  zpa,  zpa,  inv,  imp,  imm,  imp,  inv, abso, abso, abso,  inv,
/*F0*/   rel, zpiy,  inv,  inv,  inv,  zpx,  zpx,  inv,  imp, absy,  inv,  inv,  inv, absx, absx,  inv
};

const int util::addr_mode_byte_length[] = {
        0,
        1,
        1,
        2,
        3,
        2,
        2,
        3,
        3,
        2,
        2,
        3,
        2,
        2
    };

