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
        void set_acc(int);
        void set_x(int);
        void print_details();
private:
        void debug_dummy(int,const char *,...);
        void debug_dummy(const char *,...);
        void set_sign(unsigned char); 
        void set_zero(unsigned char);
        void set_carry(unsigned char);
        void set_verflow(unsigned char, unsigned char);
        char * stat_string(statreg_t status);
        int zp_xc, zp_yc, ind_xc, ind_yc, zpc, immediatec, absac, relativec, absa_yc, absa_xc, indc, impc, accumc;
        int zp_x();
        int zp_y();
        int ind_x();
        int ind_y();
        int zp();
        int immediate();
        int absa();
        signed char relative();
        int absa_y();
        int absa_x();
        int ind();
        void imp();
        void accum();

        void op_bpl(signed char);
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
        void op_bmi(signed char);
        void op_sec();
        void op_rti();
        void op_eor(int);
        void op_lsrm(int);
        void op_pha();
        void op_lsra();
        void op_jmp(int);
        void op_bvc(signed char);
        void op_cli();
        void op_rts();
        void op_adc(int);
        void op_rorm(int);
        void op_pla();
        void op_rora();
        void op_bvs(signed char);
        void op_sei();
        void op_sty(int);
        void op_sta(int);
        void op_stx(int);
        void op_dey();
        void op_txa();
        void op_bcc(signed char);
        void op_tya();
        void op_txs();
        void op_ldy(int);
        void op_lda(int);
        void op_ldx(int);
        void op_tay();
        void op_tax();
        void op_bcs(signed char);
        void op_clv();
        void op_tsx();
        void op_cpy(int);
        void op_cmp(int);
        void op_dec(int);
        void op_iny();
        void op_dex();
        void op_bne(signed char);
        void op_cld();
        void op_cpx(int);
        void op_sbc(int);
        void op_inc(int);
        void op_inx();
        void op_nop();
        void op_beq(signed char);
        void op_sed();

        void push(unsigned char val);
        void push2(unsigned int val);
        unsigned int pop();
        unsigned int pop2();
        void core_dump();

        unsigned int pc;
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
        signed char nextopoffset;
        unsigned int nextoparg;
        unsigned int prevpc;
        unsigned int prevop;
        unsigned int prevopaddr;
        unsigned int prevoparg;
        signed char prevopoffset;
        unsigned int prevopsrun;
        unsigned int addr;
        unsigned int addr2;
        signed char offset;
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
