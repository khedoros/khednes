#pragma once
#include "mem.h"
#include "apu.h"
#include <string>

class cpu {

public:
        union statreg_t {
            uint8_t reg;
            struct {
                //low-order
                unsigned carry:1;
                unsigned zero:1;
                unsigned inter:1;
                unsigned dec:1;
                unsigned brk:1;
                unsigned tru:1;
                unsigned verflow:1;
                unsigned sign:1;
                //high-order
            };
        };

        cpu(mem * data, apu * ap, const unsigned int start_loc, const bool is_nsf);
        ~cpu();
        const int run_next_op();
        const int run_ops();
        void trigger_nmi();
        void trigger_irq();
        void reset(int);
        void increment_frame();
        void set_ppu_cycle(int);
        void set_acc(int);
        void set_x(int);
        void print_details(const std::string&);
private:
        void debug_dummy(int,const char *,...);
        void debug_dummy(const char *,...);
        void set_sign(unsigned char); 
        void set_zero(unsigned char);
        void set_carry(unsigned char);
        void set_verflow(unsigned char, unsigned char);
        char * stat_string(statreg_t status);
        int zp_x();
        int zp_y();
        int ind_x();
        int ind_y();
        int zp();
        int immediate();
        int absa();
        int relative();
        int absa_y();
        int absa_x();
        int ind();
        int imp();
        int accum();

        enum addr_mode {
            ZERO_PAGE,
            ZERO_PAGE_X,
            ZERO_PAGE_Y,
            INDIRECT,
            INDIRECT_X,
            INDIRECT_Y,
            IMMEDIATE,
            ABSOLUTE,
            ABSOLUTE_X,
            ABSOLUTE_Y,
            RELATIVE,
            IMPLIED,
            ACCUMULATOR,
            INVALID_MODE
        };

        void op_bpl(int);
        void op_brk();        
        void op_ora(int);
        void op_aslm(int);
        void op_asla();
        void op_php();
        void op_clc();
        void op_jsr(int);
        void op_bit(int);
        void op_and(int);
        void op_rolm(int);
        void op_plp();
        void op_rola();
        void op_bmi(int);
        void op_sec();
        void op_rti();
        void op_eor(int);
        void op_lsrm(int);
        void op_pha();
        void op_lsra();
        void op_jmp(int);
        void op_bvc(int);
        void op_cli();
        void op_rts();
        void op_adc(int);
        void op_rorm(int);
        void op_pla();
        void op_rora();
        void op_bvs(int);
        void op_sei();
        void op_sty(int);
        void op_sta(int);
        void op_stx(int);
        void op_dey();
        void op_txa();
        void op_bcc(int);
        void op_tya();
        void op_txs();
        void op_ldy(int);
        void op_lda(int);
        void op_ldx(int);
        void op_tay();
        void op_tax();
        void op_bcs(int);
        void op_clv();
        void op_tsx();
        void op_cpy(int);
        void op_cmp(int);
        void op_dec(int);
        void op_iny();
        void op_dex();
        void op_bne(int);
        void op_cld();
        void op_cpx(int);
        void op_sbc(int);
        void op_inc(int);
        void op_inx();
        void op_nop();
        void op_beq(int);
        void op_sed();

        enum operation {
            BPL,
            BRK,        
            ORA,
            ASLM,
            ASLA,
            PHP,
            CLC,
            JSR,
            BIT,
            AND,
            ROLM,
            PLP,
            ROLA,
            BMI,
            SEC,
            RTI,
            EOR,
            LSRM,
            PHA,
            LSRA,
            JMP,
            BVC,
            CLI,
            RTS,
            ADC,
            RORM,
            PLA,
            RORA,
            BVS,
            SEI,
            STY,
            STA,
            STX,
            DEY,
            TXA,
            BCC,
            TYA,
            TXS,
            LDY,
            LDA,
            LDX,
            TAY,
            TAX,
            BCS,
            CLV,
            TSX,
            CPY,
            CMP,
            DEC,
            INY,
            DEX,
            BNE,
            CLD,
            CPX,
            SBC,
            INC,
            INX,
            NOP,
            BEQ,
            SED,
            INVALID_OP
        };

        void push(unsigned char val);
        void push2(unsigned int val);
        unsigned int pop();
        unsigned int pop2();
        void core_dump();

        int pc;
        unsigned char acc;
        statreg_t status;
        unsigned char x;
        unsigned char y;
        unsigned char sp;

        mem * memory;
        apu * audio;

        int inst_counts[256];
        const static bool impd[];
        const static unsigned int runtime[256];

        unsigned int nextop;        
        unsigned int nextopaddr;
        int nextopoffset;
        unsigned int nextoparg;
        unsigned int prevpc;
        unsigned int prevop;
        unsigned int prevopaddr;
        unsigned int prevoparg;
        int prevopoffset;
        unsigned int prevopsrun;
        unsigned int addr;
        unsigned int addr2;
        int offset;
        //unsigned char temp;
        //unsigned char temp2;
        unsigned int temp;
        unsigned int temp2;
        unsigned int extra_time;
        unsigned int ppu_cycle;
        unsigned int ops_ahead;
        unsigned int frame;
        bool first;
        /*
        char op_addr[9];
        char raw_op[12];
        char assem_op[14];
        */
        char op_addr[30];
        char raw_op[30];
        char assem_op[30];

        char st[9];
        char stat_str[87];

        const static int FLAG_CARRY=0x01;
        const static int FLAG_ZERO=0x02;
        const static int FLAG_INT=0x04;
        const static int FLAG_DEC=0x08;
        const static int FLAG_BRK=0x10;
        const static int FLAG_TRUE=0x20;
        const static int FLAG_VERFLOW=0x40;
        const static int FLAG_SIGN=0x80;

        bool nsf_mode;
};
