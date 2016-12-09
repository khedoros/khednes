#include<iostream>
#include<iomanip>
#include "rom.h"
#include<list>
#include<vector>
using namespace std;

enum inst_types {none,        //00
                 implied,     //01
                 indirect_x,  //02
                 zero_page,   //03
                 immediate,   //04
                 accumulator, //05
                 absolute,    //06
                 relative,    //07
                 indirect_y,  //08
                 zero_page_x, //09
                 absolute_y,  //10
                 absolute_x,  //11
                 indirect,    //12
                 zero_page_y};//13

const inst_types inst_type[] = {
                        implied,   indirect_x, none,      none, none,        zero_page,   zero_page,   none, implied, immediate,  accumulator, none, none,       absolute,   absolute,   none,
                        relative,  indirect_y, none,      none, none,        zero_page_x, zero_page_x, none, implied, absolute_y, none,        none, none,       absolute_x, absolute_x, none,
                        absolute,  indirect_x, none,      none, zero_page,   zero_page,   zero_page,   none, implied, immediate,  accumulator, none, absolute,   absolute,   absolute,   none,
                        relative,  indirect_y, none,      none, none,        zero_page_x, zero_page_x, none, implied, absolute_y, none,        none, none,       absolute_x, absolute_x, none,
                        implied,   indirect_x, none,      none, none,        zero_page,   zero_page,   none, implied, immediate,  accumulator, none, absolute,   absolute,   absolute,   none,
                        relative,  indirect_y, none,      none, none,        zero_page_x, zero_page_x, none, implied, absolute_y, none,        none, none,       absolute_x, absolute_x, none,
                        implied,   indirect_x, none,      none, none,        zero_page,   zero_page,   none, implied, immediate,  accumulator, none, indirect,   absolute,   absolute,   none,
                        relative,  indirect_y, none,      none, none,        zero_page_x, zero_page_x, none, implied, absolute_y, none,        none, none,       absolute_x, absolute_x, none,
                        none,      indirect_x, none,      none, zero_page,   zero_page,   zero_page,   none, implied, none,       implied,     none, absolute,   absolute,   absolute,   none,
                        relative,  indirect_y, none,      none, zero_page_x, zero_page_x, zero_page_y, none, implied, absolute_y, implied,     none, none,       absolute_x, none,       none,
                        immediate, indirect_x, immediate, none, zero_page,   zero_page,   zero_page,   none, implied, immediate,  implied,     none, absolute,   absolute,   absolute,   none,
                        relative,  indirect_y, none,      none, zero_page_x, zero_page_x, zero_page_y, none, implied, absolute_y, implied,     none, absolute_x, absolute_x, absolute_y, none,
                        immediate, indirect_x, none,      none, zero_page,   zero_page,   zero_page,   none, implied, immediate,  implied,     none, absolute,   absolute,   absolute,   none,
                        relative,  indirect_y, none,      none, none,        zero_page_x, zero_page_x, none, implied, absolute_y, none,        none, none,       absolute_x, absolute_x, none,
                        immediate, indirect_x, none,      none, zero_page,   zero_page,   zero_page,   none, implied, immediate,  implied,     none, absolute,   absolute,   absolute,   none,
                        relative,  indirect_y, none,      none, none,        zero_page_x, zero_page_x, none, implied, absolute_y, none,        none, none,       absolute_x, absolute_x, none};

const char name[256][4] = {
                      "BRK","ORA","xxx","xxx",  "xxx","ORA","ASL","xxx",  "PHP","ORA","ASL","xxx",  "xxx","ORA","ASL","xxx",
                      "BPL","ORA","xxx","xxx",  "xxx","ORA","ASL","xxx",  "CLC","ORA","xxx","xxx",  "xxx","ORA","ASL","xxx",
                      "JSR","AND","xxx","xxx",  "BIT","AND","ROL","xxx",  "PLP","AND","ROL","xxx",  "BIT","AND","ROL","xxx",
                      "BMI","AND","xxx","xxx",  "xxx","AND","ROL","xxx",  "SEC","AND","xxx","xxx",  "xxx","AND","ROL","xxx",

                      "RTI","EOR","xxx","xxx",  "xxx","EOR","LSR","xxx",  "PHA","EOR","LSR","xxx",  "JMP","EOR","LSR","xxx",
                      "BVC","EOR","xxx","xxx",  "xxx","EOR","LSR","xxx",  "CLI","EOR","xxx","xxx",  "xxx","EOR","LSR","xxx",
                      "RTS","ADC","xxx","xxx",  "xxx","ADC","ROR","xxx",  "PLA","ADC","ROR","xxx",  "JMP","ADC","ROR","xxx",
                      "BVS","ADC","xxx","xxx",  "xxx","ADC","ROR","xxx",  "SEI","ADC","xxx","xxx",  "xxx","ADC","ROR","xxx",

                      "xxx","STA","xxx","xxx",  "STY","STA","STX","xxx",  "DEY","xxx","TXA","xxx",  "STY","STA","STX","xxx",
                      "BCC","STA","xxx","xxx",  "STY","STA","STX","xxx",  "TYA","STA","TXS","xxx",  "xxx","STA","xxx","xxx",
                      "LDY","LDA","LDX","xxx",  "LDY","LDA","LDX","xxx",  "TAY","LDA","TAX","xxx",  "LDY","LDA","LDX","xxx",
                      "BCS","LDA","xxx","xxx",  "LDY","LDA","LDX","xxx",  "CLV","LDA","TSX","xxx",  "LDY","LDA","LDX","xxx",

                      "CPY","CMP","xxx","xxx",  "CPY","CMP","DEC","xxx",  "INY","CMP","DEX","xxx",  "CPY","CMP","DEC","xxx",
                      "BNE","CMP","xxx","xxx",  "xxx","CMP","DEC","xxx",  "CLD","CMP","xxx","xxx",  "xxx","CMP","DEC","xxx",
                      "CPX","SBC","xxx","xxx",  "CPX","SBC","INC","xxx",  "INX","SBC","NOP","xxx",  "CPX","SBC","INC","xxx",
                      "BEQ","SBC","xxx","xxx",  "xxx","SBC","INC","xxx",  "SED","SBC","xxx","xxx",  "xxx","SBC","INC","xxx" };

const int inst_size(const int op) {
    switch(inst_type[op]) {
        case implied:
        case accumulator:
            return 1;
        case indirect_x:
        case indirect_y:
        case zero_page:
        case zero_page_x:
        case zero_page_y:
        case immediate:
        case relative:
            return 2;
        case absolute:
        case absolute_y:
        case absolute_x:
        case indirect:
            return 3;
        default: return 0;
    }
}

void print_inst(rom& game, const unsigned int addr) {
    unsigned int op = game.get_pbyte(addr);
    cout<<hex<<setw(4)<<addr<<"\t";
    switch(inst_size(op)) {
        case 0: cout<<hex<<setw(2)<<op<<"       "; break;
        case 1: cout<<hex<<setw(2)<<op<<"       "; break;
        case 2: cout<<hex<<setw(2)<<op<<" "<<setw(2)<<game.get_pbyte(addr+1)<<"    "; break;
        case 3: cout<<hex<<setw(2)<<op<<" "<<setw(2)<<game.get_pbyte(addr+1)<<" "<<setw(2)<<game.get_pbyte(addr+2)<<" "; break;
    }
    cout<<"\t"<<name[op];
    switch(inst_type[op]) {
        case indirect_x:
            cout<<" ($"<<hex<<game.get_pbyte(addr+1)<<", X)"<<endl;
            break;
        case zero_page:
            cout<<" $"<<hex<<game.get_pbyte(addr+1)<<endl;
            break;
        case immediate:
            cout<<" #$"<<hex<<game.get_pbyte(addr+1)<<endl;
            break;
        case accumulator:
            cout<<" A"<<endl;
            break;
        case absolute:
            cout<<" $"<<hex<<game.get_pword(addr+1)<<endl;
            break;
        case relative: {
            char offset = game.get_pbyte(addr+1);
            cout<<" $"<<hex<<addr+offset+2<<endl;
                       }
            break;
        case indirect_y:
            cout<<" ($"<<hex<<game.get_pbyte(addr+1)<<"), Y"<<endl;
            break;
        case zero_page_x:
            cout<<" $"<<hex<<game.get_pbyte(addr+1)<<", X"<<endl;
            break;
        case absolute_y:
            cout<<" $"<<hex<<game.get_pword(addr+1)<<", Y"<<endl;
            break;
        case absolute_x:
            cout<<" $"<<hex<<game.get_pword(addr+1)<<", X"<<endl;
            break;
        case indirect:
            cout<<" ($"<<hex<<game.get_pword(addr+1)<<")"<<endl;
            break;
        case zero_page_y:
            cout<<" $"<<hex<<game.get_pbyte(addr+1)<<", Y"<<endl;
            break;
        default: //implied+none
            cout<<endl;
            return;
    }
}

unsigned int successor(rom& game, const unsigned int addr) {
    int op = game.get_pbyte(addr);
    if(inst_type[op] == none) return 0;
    switch(op) {
        case 0x00: return addr+2; //BRK
        case 0x4c: return game.get_pword(addr+1); //ABS JMP
        case 0x40:                                     //RTI       \
        case 0x60:                                     //RTS       |----hard to predict
        case 0x6c:                                     //IND JMP   /
                   return 0;
        default:
                   return addr+inst_size(op);
    }
    return 0;
}

unsigned int alt_successor(rom& game, const unsigned int addr) {
    int op = game.get_pbyte(addr);
    switch(op) {
        case 0x10: //  \
        case 0x30: //  |
        case 0x50: //  |
        case 0x70: //  |
        case 0x90: //  |----relative jumps
        case 0xB0: //  |
        case 0xD0: //  |
        case 0xF0: //  /
            {
                char offset = game.get_pbyte(addr+1);
                return addr+offset+2;
            }
        case 0x20: return game.get_pword(addr+1); //JSR
        default: return 0;
    }
    return 0;
}

int main() {
    rom game("duckhunt.nes",0);
    if(game.isValid())
        game.print_info();
    else
        return 1;
    std::list<unsigned int> addrs;
    std::vector<bool> visited(0x8000,false);
    ifstream addr_file("addresses");
    if(addr_file.is_open()) {
        int addr_inp = 0;
        int addr_count = 0;
        addr_file>>setbase(16);
        cout<<hex;
        while(!addr_file.eof()) {
            addr_file>>addr_inp;
            addr_count++;
            //cout<<addr_inp<<endl;
            addrs.push_back(addr_inp);
        }
        cout<<"Took input of "<<dec<<addr_count<<" addresses from the file."<<endl;
    }
    else {
        addrs.push_back(game.get_rst_addr());
        addrs.push_back(game.get_nmi_addr());
        addrs.push_back(game.get_irq_addr());
    }

    while(addrs.size() > 0) {
        unsigned int addr = addrs.front();
        addrs.pop_front();
        if(visited[addr-0x8000]) continue;
        visited[addr-0x8000] = true;
        int s = successor(game,addr);
        int as = alt_successor(game, addr);

        //cout<<"At "<<hex<<addr;
        if(s) {
            addrs.push_back(s);
            //cout<<", planning to visit "<<s;
        }
        if(as) {
            addrs.push_back(as);
            //cout<<", planning to visit "<<as;
        }
        //cout<<endl;
    }
    int visited_addrs = 0;
    for(int i=0x4000;i<visited.size();++i) {
        if(visited[i]) {
            print_inst(game,i+0x8000);
            visited_addrs++;
        }
        else cout<<hex<<i+0x8000<<"\t"<<game.get_pbyte(i+0x8000)<<endl;
    }
    cout<<dec<<"Visited "<<visited_addrs<<" addresses after searching the conditionals"<<endl;
    return 0;
}
