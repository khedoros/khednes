#pragma once
#include<stdio.h>
#include<stdarg.h>
#include<vector>
#include<bitset>
#include<string>

template<typename T>
class RangeCheckVector : public std::vector<T> {
public:
    T& operator[](int i) {
        return std::vector<T>::at(i);
    }

    const T& operator[](int i) const {
        return std::vector<T>::at(i);
    }
};

#ifdef DEBUG
#define Vect RangeCheckVector
#else
#define Vect std::vector
#endif

class util {
public:
    static int debug(int orig, const char * fmt,...);
    static int debug(const char * fmt,...);
    static void toggle_log(int origin);
    static bool is_active(int i);
    static std::string inst_string(int byte0, int byte1=-1, int byte2=-1);

    static const std::string inst_names[];
private:
    static std::bitset<1000> active;
    static bool init;
};

//       00     01     02     03     04     05     06     07     08     09     0A     0B     0C     0D     0E     0F
const std::string inst_names[] {
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
