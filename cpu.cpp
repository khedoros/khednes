#include "cpu.h"
#include "util.h"
#include <iostream>
#include <fstream>
#define ORIGIN 1
using namespace std;

#ifdef _WIN32
#define strncat strncat_s
#endif
void cpu::debug_dummy(int origin,const char * blah,...) {
    return;
}

void cpu::debug_dummy(const char *,...) {
    return;
}

Vect<Vect<int>> addresses;

cpu::cpu(mem * data, apu * ap, const unsigned int start_loc, bool is_nsf) : nsf_mode(is_nsf), max(20) {
    for(int i=0;i<256;++i) {
        inst_counts[i]=0;
    }
    memory=data;
    audio=ap;
    pc=start_loc;
    status.reg=FLAG_TRUE|FLAG_BRK;
    sp=0x00;
    acc=0;
    x=0;
    y=0;
    ops_ahead=0;
    ppu_cycle=0;
    addr=0;
    prevpc=0;
    prevop=0;
    prevopaddr = 0;
    prevopoffset = 0;
    prevoparg = 0;
    nextop = 0;
    nextopaddr = 0;
    nextopoffset = 0;
    nextoparg = 0;
    frame = 0;

    addresses.resize(17); //17th page is for any PC values that occur in RAM instead of ROM
    for(int page=0;page<16;page++) {
        addresses[page].resize(PRG_PAGE_SIZE);
        for(int paddr=0;paddr<PRG_PAGE_SIZE;paddr++) {
            addresses[page][paddr] = 0;
        }
    }
    addresses[16].resize(0x8000);
    for(int paddr=0;paddr<0x8000;paddr++) {
        addresses[16][paddr] = 0;
    }
}

void cpu::print_details(const std::string& filename) {
    int total=0;
    int count=0;

    for(int i=0;i<256;++i) {
        if(inst_counts[i]>0) {
            printf("Saw instruction #%02x  %d times.\n",i,inst_counts[i]);
            total+=inst_counts[i];
            count++;
        }
    }
    printf("Total of %d instructions invoked a total of %d times.\n",count,total);

    if(filename == "") return;
    ifstream rom(filename);
    bool print_insts = true;
    size_t filesize = 0;
    Vect<uint8_t> data;
    if(!rom.is_open()) {
        print_insts = false;
        cout<<"wtf couldn't open "<<filename<<" grrrr"<<endl;
    }
    else {
        cout<<"Opened "<<filename<<". ";
        rom.seekg(0,ios::end);
        filesize = rom.tellg() - streampos(16);
        cout<<"Going to read "<<dec<<filesize<<" bytes from it."<<endl;
        rom.seekg(16,ios::beg);
        if(filesize < 40 * 1024) {
            data.resize(filesize+PRG_PAGE_SIZE);
            rom.read(reinterpret_cast<char *>(&data[0]), filesize);
            rom.seekg(16,ios::beg);
            rom.read(reinterpret_cast<char *>(&data[PRG_PAGE_SIZE]), filesize);
        }
        else {
            data.resize(filesize);
            rom.read(reinterpret_cast<char *>(&data[0]), filesize);
        }
        rom.close();
        cout<<"Read the data. Closed the file."<<endl;
    }
    cout.flush();
    int addr_count = 0;
    for(int j=0;j<0x8000;++j) {
        if(addresses[16][j]) {
            if(print_insts) {
                printf("RAM:%04X (%04X)\t%s\n", j, addresses[16][j], util::inst_string(memory->read(j),memory->read(j+1),memory->read(j+2)).c_str());
            }
            else {
                printf("RAM:%04X (%04X)\n", j, addresses[16][j]);
            }
            addr_count++;
        }
    }
    for(int i=0;i<16;++i) {
        for(int j=0;j<PRG_PAGE_SIZE;++j) {
            if(addresses[i][j]) {
                if(print_insts) {
                    int base = i * PRG_PAGE_SIZE + j;
                    if(base < data.size()) {
                        printf("%02X:%04X (%04X)\t%s\n", i, j, addresses[i][j], (util::inst_string(data[base],data[base+1],data[base+2])).c_str());
                    }
                    else {
                        printf("%02X:%04X is outside the file.\n",i,j);
                    }
                }
                else {
                    printf("%02X:%04X (%04X)\n",i,j, addresses[i][j]);
                }
                addr_count++;
            }
        }
    }
    cout<<"The code visited "<<dec<<addr_count<<" addresses during execution."<<endl;
}


cpu::~cpu() {
}

                                //      0 1 2 3  4 5 6 7  8 9 a b  c d e f
const unsigned int cpu::runtime[]={     7,6,0,0, 0,3,5,0, 3,2,2,0, 0,4,6,0,
                                        2,5,0,0, 0,4,6,0, 2,4,0,0, 0,4,7,0,
                                        6,6,0,0, 3,3,5,0, 4,2,2,0, 4,4,6,0,
                                        2,5,0,0, 0,4,6,0, 2,4,0,0, 0,4,7,0,

                                        4,6,0,0, 0,3,5,0, 3,2,2,0, 3,6,6,0,
                                        2,5,0,0, 0,4,6,0, 2,4,0,0, 0,4,7,0,
                                        6,6,0,0, 0,3,5,0, 4,2,2,0, 5,4,6,0,
                                        2,5,0,0, 0,4,6,0, 2,4,0,0, 0,4,7,0,

                                        0,6,0,0, 3,3,3,0, 2,0,2,0, 4,4,4,0,
                                        2,6,0,0, 4,4,4,0, 2,5,2,0, 0,5,0,0,
                                        2,6,2,0, 3,3,3,0, 2,2,2,0, 4,4,4,0,
                                        2,5,0,0, 4,4,4,0, 2,4,2,0, 4,4,4,0,

                                        2,6,0,0, 3,3,5,0, 2,2,2,0, 4,4,6,0,
                                        2,5,0,0, 0,4,6,0, 2,4,0,0, 0,4,7,0,
                                        2,6,0,0, 3,3,5,0, 2,2,2,0, 4,4,6,0,
                                        2,5,0,0, 0,4,6,0, 2,4,0,0, 0,4,7,0};

//void * table[256] = {

void cpu::core_dump() {
    return;
    printf("CORE DUMP:\n---------\n");
    for(int addr=0;addr<0x800;addr+=0x20) {
        printf("%04X->%04X:",addr,addr+0x1F);
        for(int addrsub=0;addrsub<32;addrsub++) {
            if(addrsub%4==0) {
                printf(" %02X",memory->read(addr+addrsub));
            }
            else {
                printf("%02X",memory->read(addr+addrsub));
            }
        }
        printf("\n");
    }
}

inline void cpu::set_sign(unsigned char dat) {
    status.sign = (dat >= 128);
}

inline void cpu::set_zero(unsigned char dat) {
    status.zero = (dat == 0);
}

inline void cpu::set_carry(unsigned char dat) {
    status.carry = (dat != 0);
}

inline void cpu::set_verflow(unsigned char dat1,unsigned char dat2) {
}

//Addressing Modes
inline int cpu::zp_x() {
    pc+=2;
    int base_addr=memory->read(pc-1);
    return (base_addr+x)&0xFF;
}

inline int cpu::zp_y() {
    pc+=2;
    int base_addr=memory->read(pc-1);
    return (base_addr+y)&0xFF;
}

inline int cpu::ind_x() {
    int addr=(memory->read(pc+1)+x)&0xFF;
    pc+=2;
    return (memory->read(addr))|((memory->read((addr+1)&0xFF))<<(8));
}

inline int cpu::ind_y() {
    int addr=memory->read(pc+1);
    pc+=2;
    return (((memory->read(addr))|((memory->read((addr+1)&0xFF))<<(8)))+y)&0xFFFF;
}

inline int cpu::zp() {
    int base_addr=memory->read(pc+1);
    pc+=2;
    return base_addr;
}

inline int cpu::immediate() {
    pc+=2;
    return pc-1;
}

inline int cpu::absa() {
    pc+=3;
    return memory->read2(pc-2);
}

inline int cpu::absa_y() {
    pc+=3;
    return (memory->read2(pc-2)+y)&0xFFFF;
}

inline int cpu::absa_x() {
    pc+=3;
    return (memory->read2(pc-2)+x)&0xFFFF;
}

inline int cpu::relative() {
    pc+=2;
    signed char offset = memory->read(pc-1);
    return pc + offset;
}

inline int cpu::ind() {
    int addr=memory->read2(pc+1);
    if((addr&0xFF)==0xFF) {
        return (memory->read(addr))|((memory->read(addr-0xFF))<<(8));
    }
    else {
        return memory->read2(addr);
    }
}

inline int cpu::imp() {
    pc++;
    return 0;
}

inline int cpu::accum() {
    pc++;
    return 0;
}

//Operations
inline void cpu::op_brk(int) {
    pc++;
    push2(pc);
    status.brk = 1;
    push(status.reg);
    status.inter = 1;
    pc=memory->read2(0xFFFE);
}

inline void cpu::op_ora(int addr) {
    temp=memory->read(addr);
    acc|=temp;
    set_sign(acc);
    set_zero(acc);
}

inline void cpu::op_aslm(int addr) {
    temp=memory->read(addr);
    set_carry(temp&0x80);
    temp<<=(1);
    set_zero(temp);
    set_sign(temp);
    memory->write(addr,temp);
}

inline void cpu::op_asla(int) {
    set_carry(acc&0x80);
    acc<<=(1);
    set_zero(acc);
    set_sign(acc);
}

inline void cpu::op_php(int) {
    push(status.reg);
}
                
inline void cpu::op_bpl(int offset) {
    if(!status.sign) {
        pc=offset;
        extra_time++;
        if((pc>>(8))!=((pc-offset)>>(8))) {
            extra_time++;
        }
    }
}

inline void cpu::op_clc(int) {
    status.carry = 0;
}

inline void cpu::op_jsr(int addr) {
    //saves address immediately BEFORE the one I want to return to!!!
    //REASON: RTS pops the stack to the PC, THEN increments PC by 1
    pc--;
    push2(pc);
    pc=addr;
}

inline void cpu::op_bit(int addr) {
    temp=memory->read(addr);
    set_zero(acc&temp);
    set_sign(temp);
    if((temp&0x40)>0)
        status.verflow = 1;
    else
        status.verflow = 0;
}

inline void cpu::op_and(int addr) {
    temp=memory->read(addr);
    acc&=temp;
    set_sign(acc);
    set_zero(acc);
}

inline void cpu::op_rolm(int addr) {
    temp=memory->read(addr);
    temp2 = status.carry;
    set_carry(temp&0x80);
    temp=((temp<<(1))+temp2);
    memory->write(addr,temp);
    set_zero(temp);
    set_sign(temp);        
}

inline void cpu::op_plp(int) {
    status.reg=pop();
}

inline void cpu::op_rola(int) {
    temp = status.carry;
    set_carry(acc&0x80);
    acc=(acc<<(1))+temp;
    set_zero(acc);
    set_sign(acc);
}

inline void cpu::op_bmi(int offset) {
    if(status.sign) {
        pc=offset;
        extra_time++;
        if((pc>>(8))!=((pc-offset)>>(8))) {
            extra_time++;
        }
    }
}

inline void cpu::op_sec(int) {
    status.carry = 1;
}

inline void cpu::op_rti(int) {
    status.reg=pop();
    pc=pop2();
}

inline void cpu::op_eor(int addr) {
    temp=memory->read(addr);
    acc^=temp;
    set_sign(acc);
    set_zero(acc);
}

inline void cpu::op_lsrm(int addr) {
    temp=memory->read(addr);
    status.sign = 0;
    set_carry(temp&0x01);
    temp>>=(1);
    set_zero(temp);
    memory->write(addr,temp);
}

inline void cpu::op_pha(int) {
    push(acc);
}

inline void cpu::op_lsra(int) {
    status.sign = 0;
    set_carry(acc&0x01);
    acc>>=(1);
    set_zero(acc);
}

inline void cpu::op_jmp(int addr) {
    pc=addr;
}

inline void cpu::op_bvc(int offset) {
    if(!status.verflow) {
        pc=offset;
        extra_time++;
        if((pc>>(8))!=((pc-offset)>>(8))) {
            extra_time++;
        }
    }
}

inline void cpu::op_cli(int) {
    status.inter = 0;
}

inline void cpu::op_rts(int) {
    pc=pop2();
    pc++;
}

inline void cpu::op_adc(int addr) {
    temp=memory->read(addr);
    temp2=temp+acc+status.carry;
    //if the operands have the same sign, AND      The operands have the same sign as the result
    if (!((acc ^ temp) & 0x80) && ((acc ^ temp2) & 0x80))
        status.verflow = 1;
    else
        status.verflow = 0;
    set_zero(temp2);
    set_sign(temp2);
    if((int(temp)+int(acc)+status.carry)>0xFF)
        status.carry = 1;
    else
        status.carry = 0;
    acc=temp2;
}

inline void cpu::op_rorm(int addr) {
    temp=memory->read(addr);
    temp2=status.carry*0x80;
    set_carry(temp&0x01);
    temp=(temp>>(1))+temp2;
    set_sign(temp);
    set_zero(temp);
    memory->write(addr,temp);
}

inline void cpu::op_pla(int) {
    acc=pop();
    set_sign(acc);
    set_zero(acc);
}

inline void cpu::op_rora(int) {
    temp2=status.carry*0x80;
    set_carry(acc&0x01);
    acc=(acc>>(1))+temp2;
    set_sign(acc);
    set_zero(acc);
}

inline void cpu::op_bvs(int offset) {
    if(status.verflow) {
        pc=offset;
        extra_time++;
        if((pc>>(8))!=((pc-offset)>>(8))) {
                extra_time++;
        }
    }
}

inline void cpu::op_sei(int) {
    status.inter = 1;
}

inline void cpu::op_sty(int addr) {
    memory->write(addr,y);
}

inline void cpu::op_sta(int addr) {
    memory->write(addr,acc);
}

inline void cpu::op_stx(int addr) {
    memory->write(addr,x);
}

inline void cpu::op_dey(int) {
    y--;
    set_sign(y);
    set_zero(y);
}

inline void cpu::op_txa(int) {
    acc=x;
    set_sign(x);
    set_zero(x);
}

inline void cpu::op_bcc(int offset) {
    if(!status.carry) {
        extra_time++;
        pc=offset;
        if((pc>>(8))!=((pc-offset)>>(8))) {
            extra_time++;
        }
    }
}

inline void cpu::op_tya(int) {
    acc=y;
    set_sign(y);
    set_zero(y);
}

inline void cpu::op_txs(int) {
    sp=x;
}

inline void cpu::op_ldy(int addr) {
        y=memory->read(addr);
        set_sign(y);
        set_zero(y);
}

inline void cpu::op_lda(int addr) {
        acc=memory->read(addr);
        set_sign(acc);
        set_zero(acc);
}

inline void cpu::op_ldx(int addr) {
        x=memory->read(addr);
        set_sign(x);
        set_zero(x);
}

inline void cpu::op_tay(int) {
    y=acc;
    set_sign(y);
    set_zero(y);
}

inline void cpu::op_tax(int) {
    x=acc;
    set_sign(x);
    set_zero(x);
}

inline void cpu::op_bcs(int offset) {
    if(status.carry) {
        extra_time++;
        pc=offset;
        if((pc>>(8))!=((pc-offset)>>(8))) {
            extra_time++;
        }
    }
}

inline void cpu::op_clv(int) {
    status.verflow = 0;
}

inline void cpu::op_tsx(int) {
    x=sp;
    set_sign(x);
    set_zero(x);
}

inline void cpu::op_cpy(int addr) {
    temp=memory->read(addr);
    if(y>=temp)
        status.carry = 1;
    else
        status.carry = 0;
    temp=y-temp;
    set_sign(temp);
    set_zero(temp);
}

inline void cpu::op_cmp(int addr) {
    temp=memory->read(addr);
    if(acc>=temp)
        status.carry = 1;
    else
        status.carry = 0;
    temp=acc-temp;
    set_sign(temp);
    set_zero(temp);
}

inline void cpu::op_dec(int addr) {
    temp=memory->read(addr);
    temp--;
    set_sign(temp);
    set_zero(temp);
    memory->write(addr,temp);
}

inline void cpu::op_iny(int) {
    y++;
    set_sign(y);
    set_zero(y);
}

inline void cpu::op_dex(int) {
    x--;
    set_sign(x);
    set_zero(x);
}

inline void cpu::op_bne(int offset) {
    if(!status.zero) {
        pc=offset;
        extra_time++;
        if((pc>>(8))!=((pc-offset)>>(8))) {
            extra_time++;
        }
    }
}

inline void cpu::op_cld(int) {
    status.dec = 0;
}

inline void cpu::op_cpx(int addr) {
    temp=memory->read(addr);
    if(x>=temp)
        status.carry = 1;
    else
        status.carry = 0;
    temp=x-temp;
    set_sign(temp);
    set_zero(temp);
}

inline void cpu::op_sbc(int addr) {
    temp=memory->read(addr);
    temp2=temp;
    temp+=(status.carry?0:1);
    set_zero(acc-temp);
    set_sign(acc-temp);
    if(((acc^temp)&0x80)>0&&((acc^temp2)&0x80)>0) //magic code from 6502 docs
        status.verflow = 1;
    else
        status.verflow = 0;

    if(acc>=temp) {
        status.carry = 1;
    }
    else {
        status.carry = 0;
    }
    acc-=temp;
}

inline void cpu::op_inc(int addr) {
    temp=memory->read(addr);
    temp++;
    set_sign(temp);
    set_zero(temp);
    memory->write(addr,temp);
}

inline void cpu::op_inx(int) {
    x++;
    set_sign(x);
    set_zero(x);
}

inline void cpu::op_nop(int) {
    
}

inline void cpu::op_beq(int offset) {
    if(status.zero) {
        pc=offset;
        extra_time++;
        if((pc>>(8))!=((pc-offset)>>(8))) {
            extra_time++;
        }
    }
}

inline void cpu::op_sed(int) {
    status.dec = 1;
}

int inst_count=0;
const int cpu::run_next_op() {
    //std::cout<<"Addr: "<<std::hex<<pc<<std::endl;
#ifdef DEBUG
    print_inst_trace();
#endif
    prevop = nextop;
    prevopaddr = nextopaddr;
    prevoparg = addr;
    prevopoffset = offset;
    memory->set_cycle(ppu_cycle);
    nextop=memory->read(pc);
    nextopaddr = pc;
    addr = 0;
    ++inst_counts[nextop];
    extra_time=0;
    if(runtime[nextop]==0) {
        util::debug("CPU found a 0-time opcode (0x%02x)!\n",nextop);
    }

    //Used to print out old cpu state when the new one has been calculated
    //int oldpc;
    statreg_t oldstatus;
    //oldpc=pc;
    oldstatus=status;
    if(pc >= 0x8000) {
        unsigned int page = memory->get_page(pc);
        if(page >= 16) cout<<"Page: "<<page<<endl;
        assert(page < 16);
        addresses[page][pc & (PRG_PAGE_SIZE - 1)] = pc;
    }
    if(pc<0x8000) {
        addresses[16][pc] = pc;
    }
     
    assem_op[0] = 0;

    switch(nextop) {
    case 0x00://BRK
        if(nsf_mode) {
            std::cout<<"NSF files shouldn't be running BRK"<<std::endl;
            return -1;
        }
        imp();
        op_brk(0);
        break;
    case 0x01://ORA IND X
        addr=ind_x();
        op_ora(addr);
        break;
    case 0x05://ORA ZP
        addr=zp();
        op_ora(addr);
        break;
    case 0x06://ZP ASL
        addr=zp();
        op_aslm(addr);
        break;
    case 0x08://PHP
        imp();
        op_php(0);
        break;
    case 0x09://ORA IMM
        addr=immediate();
        op_ora(addr);
        break;
    case 0x0a://ASL A
        accum();
        op_asla(0);
        break;
    case 0x0d://ORA ABS
        addr=absa();
        op_ora(addr);
        break;
    case 0x0e://ASL ABS
        addr=absa();
        op_aslm(addr);
        break;
    case 0x10://BPL
        offset=relative();
        op_bpl(offset);
        break;
    case 0x11://ORA IND Y
        addr=ind_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_ora(addr);
        break;
    case 0x15://ORA ZP X
        addr=zp_x();
        op_ora(addr);
        break;
    case 0x16://ASL ZP X
        addr=zp_x();
        op_aslm(addr);
        break;
    case 0x18://CLC
        imp();
        op_clc(0);
        break;
    case 0x19://ORA ABS Y
        addr=absa_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_ora(addr);
        break;
    case 0x1d://ORA ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_ora(addr);
        break;
    case 0x1e://ASL ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_aslm(addr);
        break;
    case 0x20://JSR
        addr=absa();
        op_jsr(addr);
        break;
    case 0x21://AND IND X
        addr=ind_x();
        op_and(addr);
        break;
    case 0x24://BIT ZP
        addr=zp();
        op_bit(addr);
        break;
    case 0x25://AND ZP
        addr=zp();
        op_and(addr);
        break;
    case 0x26://ROL ZP
        addr=zp();
        op_rolm(addr);
        break;
    case 0x28://PLP
        imp();
        op_plp(0);
        break;
    case 0x29://AND IMM
        addr=immediate();
        op_and(addr);
        break;
    case 0x2a://ROL A
        accum();
        op_rola(0);
        break;
    case 0x2c://BIT ABS
        addr=absa();
        op_bit(addr);
        break;
    case 0x2d://AND ABS
        addr=absa();
        op_and(addr);
        break;
    case 0x2e:
        addr=absa();
        op_rolm(addr);
        break;
    case 0x30://BMI
        offset=relative();
        op_bmi(offset);
        break;
    case 0x31://AND IND Y
        addr=ind_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_and(addr);
        break;
    case 0x35://AND ZP X
        addr=zp_x();
        op_and(addr);
        break;
    case 0x36:
        addr=zp_x();
        op_rolm(addr);
        break;
    case 0x38://SEC
        imp();
        op_sec(0);
        break;
    case 0x39://AND ABS Y
        addr=absa_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_and(addr);
        break;
    case 0x3d://AND ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_and(addr);
        break;
    case 0x3e://ROL ABS X
        addr=absa_x();
        op_rolm(addr);
        break;
    case 0x40://RTI
        if(nsf_mode) {
            return 0;
        }
        //if(nsf_mode) return 0;
        imp();
        op_rti(0);
        break;
    case 0x41://EOR IND X
        addr=ind_x();
        op_eor(addr);
        break;
    case 0x45://EOR ZP
        addr=zp();
        op_eor(addr);
        break;
    case 0x46://LSR ZP
        addr=zp();
        op_lsrm(addr);
        break;
    case 0x48://PHA
        imp();
        op_pha(0);
        break;
    case 0x49://EOR IMM
        addr=immediate();
        op_eor(addr);
        break;
    case 0x4a://LSR A
        accum();
        op_lsra(0);
        break;
    case 0x4c://JMP ABS
        addr=absa();
        op_jmp(addr);
        break;
    case 0x4d://EOR ABS
        addr=absa();
        op_eor(addr);
        break;
    case 0x4e://LSR ABS
        addr=absa();
        op_lsrm(addr);
        break;
    case 0x50://BVC
        offset=relative();
        op_bvc(offset);
        break;
    case 0x51://EOR IND Y
        addr=ind_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_eor(addr);
        break;
    case 0x55://EOR ZP X
        addr=zp_x();
        op_eor(addr);
        break;
    case 0x56://LSR ZP X
        addr=zp_x();
        op_lsrm(addr);
        break;
    case 0x58://CLI
        imp();
        op_cli(0);
        break;
    case 0x59://EOR ABS Y
        addr=absa_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_eor(addr);
        break;
    case 0x5d://EOR ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_eor(addr);
        break;
    case 0x5e://LSR ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_lsrm(addr);
        break;
    case 0x60://RTS
        imp();
        if(nsf_mode) {
            int tempaddr=pop2();
            sp-=2;
            if(tempaddr<0x8000) {
                //printf("Returning because proposed return address is %04x\n", tempaddr);
                return 0;
            }
        }
        op_rts(0);
        break;
    case 0x61://ADC IND X
        addr=ind_x();
        op_adc(addr);
        break;
    case 0x65://ADC ZP
        addr=zp();
        op_adc(addr);
        break;
    case 0x66://ROR ZP
        addr=zp();
        op_rorm(addr);
        break;
    case 0x68://PLA
        imp();
        op_pla(0);
        break;
    case 0x69://ADC IMM
        addr=immediate();
        op_adc(addr);
        break;
    case 0x6a://ROR A
        accum();
        op_rora(0);
        break;
    case 0x6c://JMP IND
        addr=ind();
        op_jmp(addr);
        break;
    case 0x6d://ADC ABS
        addr=absa();
        op_adc(addr);
        break;
    case 0x6e://ROR ABS
        addr=absa();
        op_rorm(addr);
        break;
    case 0x70://BVS
        offset=relative();
        op_bvs(offset);
        break;
    case 0x71://ADC IND Y
        addr=ind_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_adc(addr);
        break;
    case 0x75://ADC ZP X
        addr=zp_x();
        op_adc(addr);
        break;
    case 0x76://ROR ZP X
        addr=zp_x();
        op_rorm(addr);
        break;
    case 0x78://SEI
        imp();
        op_sei(0);
        break;
    case 0x79://ADC ABS Y
        addr=absa_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_adc(addr);
        break;
    case 0x7d://ADC ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_adc(addr);
        break;
    case 0x7e://ROR ABS X
        addr=absa_x();
        op_rorm(addr);
        break;
    case 0x81://STA IND X
        addr=ind_x();
        op_sta(addr);
        break;
    case 0x84://STY ZP
        addr=zp();
        op_sty(addr);
        break;
    case 0x85://STA ZP
        addr=zp();
        op_sta(addr);
        break;
    case 0x86://STX ZP
        addr=zp();
        op_stx(addr);
        break;
    case 0x88://DEY
        imp();
        op_dey(0);
        break;
    case 0x8a://TXA
        imp();
        op_txa(0);
        break;
    case 0x8c://STY ABS
        addr=absa();
        op_sty(addr);
        break;
    case 0x8d://STA ABS
        addr=absa();
        op_sta(addr);
        break;
    case 0x8e://STX ABS
        addr=absa();
        op_stx(addr);
        break;
    case 0x90://BCC
        offset=relative();
        op_bcc(offset);
        break;
    case 0x91://STA IND Y
        addr=ind_y();
        op_sta(addr);
        break;
    case 0x94://STY ZP X
        addr=zp_x();
        op_sty(addr);
        break;
    case 0x95://STA ZP X
        addr=zp_x();
        op_sta(addr);
        break;
    case 0x96://STX ZP Y
        addr=zp_y();
        op_stx(addr);
        break;
    case 0x98://TYA
        imp();
        op_tya(0);
        break;
    case 0x99://STA ABS Y
        addr=absa_y();
        op_sta(addr);
        break;
    case 0x9a://TXS
        imp();
        op_txs(0);
        break;
    case 0x9d://STA ABS X
        addr=absa_x();
        op_sta(addr);
        break;
    case 0xa0://LDY IMM
        addr=immediate();
        op_ldy(addr);
        break;
    case 0xa1://LDA IND X
        addr=ind_x();
        op_lda(addr);
        break;
    case 0xa2://LDX IMM
        addr=immediate();
        op_ldx(addr);
        break;
    case 0xa4://LDY ZP
        addr=zp();
        op_ldy(addr);
        break;
    case 0xa5://LDA ZP
        addr=zp();
        op_lda(addr);
        break;
    case 0xa6://LDX ZP
        addr=zp();
        op_ldx(addr);
        break;
    case 0xa8://TAY
        imp();
        op_tay(0);
        break;
    case 0xa9://LDA IMM
        addr=immediate();
        op_lda(addr);
        break;
    case 0xaa://TAX
        imp();
        op_tax(0);
        break;
    case 0xac://LDY ABS
        addr=absa();
        op_ldy(addr);
        break;
    case 0xad://LDA ABS
        addr=absa();
        op_lda(addr);
        break;
    case 0xae://LDX ABS
        addr=absa();
        op_ldx(addr);
        break;
    case 0xb0://BCS
        offset=relative();
        op_bcs(offset);
        break;
    case 0xb1://LDA IND Y
        addr=ind_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_lda(addr);
        break;
    case 0xb4://LDY ZP X
        addr=zp_x();
        op_ldy(addr);
        break;
    case 0xb5://LDA ZP X
        addr=zp_x();
        op_lda(addr);
        break;
    case 0xb6://LDX ZP Y
        addr=zp_y();
        op_ldx(addr);
        break;
    case 0xb8://CLV
        imp();
        op_clv(0);
        break;
    case 0xb9://LDA ABS Y
        addr=absa_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_lda(addr);
        break;
    case 0xba://TSX
        imp();
        op_tsx(0);
        break;
    case 0xbc://LDY ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_ldy(addr);
        break;
    case 0xbd://LDA ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_lda(addr);
        break;
    case 0xbe://LDX ABS Y
        addr=absa_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_ldx(addr);
        break;
    case 0xc0://CPY IMM
        addr=immediate();
        op_cpy(addr);
        break;
    case 0xc1://IND X CMP
        addr=ind_x();
        op_cmp(addr);
        break;
    case 0xc4://CPY ZP
        addr=zp();
        op_cpy(addr);
        break;
    case 0xc5://CMP ZP
        addr=zp();
        op_cmp(addr);
        break;
    case 0xc6://DEC ZP
        addr=zp();
        op_dec(addr);
        break;
    case 0xc8://INY
        imp();
        op_iny(0);
        break;
    case 0xc9://CMP IMM
        addr=immediate();
        op_cmp(addr);
        break;
    case 0xca://DEX
        imp();
        op_dex(0);
        break;
    case 0xcc://CPY ABS
        addr=absa();
        op_cpy(addr);
        break;
    case 0xcd://CMP ABS
        addr=absa();
        op_cmp(addr);
        break;
    case 0xce://DEC ABS
        addr=absa();
        op_dec(addr);
        break;
    case 0xd0://BNE
        offset=relative();
        op_bne(offset);
        break;
    case 0xd1://CMP IND Y
        addr=ind_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_cmp(addr);
        break;
    case 0xd5://CMP ZP X
        addr=zp_x();
        op_cmp(addr);
        break;
    case 0xd6://DEC ZP X
        addr=zp_x();
        op_dec(addr);
        break;
    case 0xd8://CLD
        imp();
        op_cld(0);
        break;
    case 0xd9://CMP ABS Y
        addr=absa_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_cmp(addr);
        break;
    case 0xdd://CMP ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_cmp(addr);
        break;
    case 0xde:
        addr=absa_x();
        op_dec(addr);
        break;
    case 0xe0://CPX IMM
        addr=immediate();
        op_cpx(addr);
        break;
    case 0xe1://SBC IND X
        addr=ind_x();
        op_sbc(addr);
        break;
    case 0xe4://CPX ABS
        addr=zp();
        op_cpx(addr);
        break;
    case 0xe5://SBC ZP
        addr=zp();
        op_sbc(addr);
        break;
    case 0xe6://INC ZP
        addr=zp();
        op_inc(addr);
        break;
    case 0xe8://INX
        imp();
        op_inx(0);
        break;
    case 0xe9://SBC IMM
        addr=immediate();
        op_sbc(addr);
        break;
    case 0xea://NOP
        imp();
        op_nop(0);
        break;
    case 0xec://CPX ABS
        addr=absa();
        op_cpx(addr);
        break;
    case 0xed://SBC ABS
        addr=absa();
        op_sbc(addr);
        break;
    case 0xee://INC ABS
        addr=absa();
        op_inc(addr);
        break;
    case 0xf0://BEQ
        offset=relative();
        op_beq(offset);
        break;
    case 0xf1://SBC IND Y
        addr=ind_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_sbc(addr);
        break;
    case 0xf5://SBC ZP X
        addr=zp_x();
        op_sbc(addr);
        break;
    case 0xf6://INC ZP X
        addr=zp_x();
        op_inc(addr);
        break;
    case 0xf8://SED
        imp();
        op_sed(0);
        break;
    case 0xf9://SBC ABS Y
        addr=absa_y();
        if((addr>>(8))!=((addr-y)>>(8))) {
            extra_time++;
        }
        op_sbc(addr);
        break;
    case 0xfd://SBC ABS X
        addr=absa_x();
        if((addr>>(8))!=((addr-x)>>(8))) {
            extra_time++;
        }
        op_sbc(addr);
        break;
    case 0xfe://INC ABS X
        addr=absa_x();
        op_inc(addr);
        break;
    default:
        printf("\n\n%04x: %02x Unimplemented opcode. I am a leaf on the wind, watch how I **HURKK**!\n",pc,nextop);
        core_dump();
        return -1;
    }
    nextopoffset = offset;
    nextoparg = addr;

    //audio->clock_cpu(runtime[nextop]+extra_time);
    return runtime[nextop]+extra_time;
}

const int cpu::run_ops() {
    memory->set_frame(frame);
    //util::debug("Running CPU. PPU will start at cycle %d.\n",ppu_cycle);
    int ops_run=0;
    ppu_cycle=0;
    unsigned int cycles_this_frame = memory->get_ppu_cycles(frame);
    while(ppu_cycle<cycles_this_frame) {
        prevopsrun = ops_run;
        ops_run=run_next_op();
        if(ops_run<0)
            return ops_run;
        ppu_cycle+=(ops_run*3);
        //cout<<hex<<"Prevop: "<<prevop<<" Prevodaddr: "<<prevopaddr<<dec<<endl;
        #ifdef SKIP_LOOPS
        if(nextopaddr == addr) { //Operation seems to be looping back on itself
            if(nextop == 0x4c || nextop == 0x6c) {
                //cout<<"Op is a jump. I'm going to jump to the end of the frame since this will repeat until NMI is hit."<<endl;
                //Remaining ppu cycles in the frame...
                unsigned int to_forward = cycles_this_frame - ppu_cycle;
                //cout<<to_forward<<"PPU cycles remaining in the frame."<<endl;
                //...divided by cycles per operation gives how many times I can run it and be within the frame.
                to_forward /= (ops_run * 3);
                //Add one more so that I'm outside of the frame.
                ++to_forward;
                //cout<<"That means I can run the operation "<<to_forward<<" times to be at cycle "<<ppu_cycle + (to_forward * 3 * ops_run)<<" out of "<<cycles_this_frame<<endl;
                ppu_cycle += (to_forward * 3 * ops_run);
            }
       }
       if (pc == prevopaddr && (nextop&0x1f) == 0x10 &&
           prevoparg <= 0x2002 && prevop >= 0xA0 && prevop <= 0xBE && prevop != 0xB0) {
            //cout<<"Looks like wait loop: prevoparg: "<<hex<<prevoparg<<" prevopaddr: "<<prevopaddr<<" pc: "<<pc<<endl;
            unsigned int to_forward = cycles_this_frame - ppu_cycle; //cycles remaining in frame
            to_forward /= ((ops_run + prevopsrun) * 3); //divided by how many cycles those two ops take together
            ppu_cycle += (to_forward * 3 * (ops_run + prevopsrun));
        }
        #endif //SKIP_LOOPS
    }
    //util::debug("I think an NMI might go here.\n");
    if(ppu_cycle>=cycles_this_frame) {
        //util::debug("Frame complete at cycle %d.\n",ppu_cycle);
        ppu_cycle-=cycles_this_frame;
    }
    frame++;
    //util::debug(ORIGIN,"About to run PPU. It should end at: %d.\n",ppu_cycle);
    return 0;
}

char * cpu::stat_string(statreg_t status) {
    if(util::is_active(ORIGIN)) {
        for(int i=0;i<9;++i) {
            st[i]=0;
        }
        strncat(st,((status.sign)?"N":"n"),1);
        strncat(st,((status.verflow)?"V":"v"),1);        
        strncat(st,((status.tru)?"T":"t"),1);        
        strncat(st,((status.brk)?"B":"b"),1);        
        strncat(st,((status.dec)?"D":"d"),1);        
        strncat(st,((status.inter)?"I":"i"),1);        
        strncat(st,((status.zero)?"Z":"z"),1);        
        strncat(st,((status.carry)?"C":"c"),1);        
    }
    return st;
}

void cpu::trigger_nmi() {
    push2(pc);
    status.inter = 0;
    push(status.reg);
    pc=memory->read2(NM_INT_INSERTION);
}

void cpu::trigger_irq() {
    //std::cout<<"IRQ TRIGGERED"<<std::endl;
    push2(pc);
    status.inter = 0;
    push(status.reg);
    pc=memory->read2(IRQ_INT_INSERTION);
}

void cpu::reset(int addr) {
    op_jmp(addr);
    //util::debug("Initiated soft reset.\n");
}

void cpu::set_x(int val) {
    x = (val & 0xff);
}

void cpu::set_acc(int val) {
    acc = (val & 0xff);
}

void cpu::set_ppu_cycle(int cycle) {
    ppu_cycle = cycle;
}

void cpu::increment_frame() {
    frame++;
    memory->set_frame(frame);
}

inline void cpu::push2(unsigned int val) {
    unsigned char pcl=pc&0xff;
    unsigned char pch=(pc/0x100)&0xff;
    //util::debug(ORIGIN, "PCL: %02x PCH:  %02x\n",pcl,pch);
    memory->write(0x100+sp,pch);
    sp--;
    memory->write(0x100+sp,pcl);
    sp--;
}

inline void cpu::push(unsigned char val) {
    memory->write(0x100+sp,val);
    sp--;
}

inline unsigned int cpu::pop2() {
    sp++;
    unsigned int pcl=memory->read(0x100+sp);
    sp++;
    unsigned int pch=memory->read(0x100+sp);
    return pch*0x100+pcl;
}

inline unsigned int cpu::pop() {
    sp++;
    return memory->read(0x100+sp);
}

void cpu::print_inst_trace() {
    std::printf("%04X  ", pc);
    unsigned char bytes[3] = {memory->read(pc), 0, 0};
    std::printf("%02X ", bytes[0]);
    for(int i=1;i<3; i++) {
        if(i<util::addr_mode_byte_length[util::inst_types[bytes[0]]]) {
            bytes[i] = memory->read(pc+i);
            std::printf("%02X ", bytes[i]);
        }
        else {
            std::printf("   ");
        }
    }
    std::string temp = util::inst_string(bytes[0], bytes[1], bytes[2]);
    if(temp.size() > max) max = temp.size();
    std::printf(" %s            ", temp.c_str());
    for(int i=0;i<max - temp.size();i++) { std::printf(" "); }
    std::printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", acc, x, y, status.reg, sp);
}

#ifdef CPUEXP
int main() {
//	cpu(mem * data, apu * ap, const unsigned int start_loc, const bool is_nsf);
apu a; rom r("potato",0); ppu p(r,1);	mem m(r,p,a); 
	cpu bob(&m,&a,0,false);
	for(int i=0;i<13;i++) {
		printf("addr mode %d: %x\n", i, bob.addrModes[i]);
	}
	for(int i=0;i<60;i++) {
		printf("operation %d: %x\n", i, bob.operations[i]);
	}
	return 0;
}
#endif
