#pragma once
#include<SDL2/SDL.h>
#include "mem.h"
#include "nes_constants.h"
#include<list>
#include "util.h"

#define SAMPLE_RATE 44100
//#define SAMPLE_RATE 22050
//#define SAMPLE_RATE 11025

class apu;
class mem;

typedef struct {
    unsigned char * buffer;
    apu * apui;
    bool first;
    int buffered;
} userdata;

typedef union {
    unsigned val:8;
    struct {
        unsigned unused:6;
        unsigned irq_disable:1;
        unsigned counter_mode:1;
    };
} frame_counter_reg;

typedef union {
    unsigned val:8;
    struct {
        unsigned vol:4;
        unsigned disable_decay:1;
        unsigned loop_decay:1;
        unsigned duty_cycle:2;
    };
} vol_reg; //envelope control, 0x4000, 0x4004, 0x400C (Noise doesn't use duty cycle)

typedef union {
    unsigned val:8;
    struct {
        unsigned shift:3;
        unsigned dir:1;
        unsigned rate:3;
        unsigned enable:1;
    };
} sweep_reg; //sweep control, 0x4001, 0x4005 (0x4009, 0x400D are invalid registers)

typedef union {
    struct {
        unsigned low:8;
        unsigned high:8;
    };
    struct {
        unsigned freq:11;
        //Values used to calculate length
        unsigned bit3:1;
        unsigned mid:3;
        unsigned bit7:1;
    };
    struct {
        unsigned noise_freq:4;
        unsigned unused1:3;
        unsigned noise_type:1;
        //Use the regular "length" fields above
        unsigned unused2:8;
    };
} freq_len_reg; //Frequency and length control (0x4002/3, 0x4006/7, 0x400A/B, 0x400E/F

typedef union {
    unsigned val:8;
    struct {
        unsigned count:7;
        unsigned length_disable:1;
    };
} lin_ctr_reg; //Linear counter value 0x4008

typedef union {
    unsigned val:8;
    struct {
        unsigned sq1_on:1;
        unsigned sq2_on:1;
        unsigned tri_on:1;
        unsigned noise_on:1;
        unsigned dmc_on:1;
        unsigned unused:3;
    };
} status_reg;

typedef union {
    unsigned val:15;
    struct {
        unsigned low:1;
        unsigned dunno:7;
        unsigned bit8:1;
        unsigned duncare:4;
        unsigned bit13:1;
        unsigned bit14:1;
    };
} shift_reg;

class apu {
public:
    apu();
    ~apu();
    void setmem(mem * memptr);
    void gen_audio(uint16_t to_gen = SAMPLE_RATE / 60);
    void reg_write(int frame, int cycle, int addr, int val);
    static const int noise_freq(freq_len_reg val);
    static const int play_length(freq_len_reg val);
    int read_status();
    SDL_AudioDeviceID get_id();
    void init();
    void generate_arrays();
    void set_frame(int f);
    int get_frame();

    bool write_pos;
    bool next_frame;
    size_t buffer_write_pos;
    size_t buffer_read_pos;
    userdata dat;
    const static uint16_t SAMPLES_PER_FRAME = SAMPLE_RATE / 60;
private:
    void reg_dequeue();
    mem * memi;
    int get_sample(int voice);
    void clock_quarter();
    void clock_half();
    void clock_full();
    void clock_every(); //clock every sample, and simulate per-cycle clocking inside
    void clock_duty(int voice);
    const int sq1_duty();
    const int sq2_duty();
    const int tri_duty();
    const int noise_duty();
    Vect<unsigned char> buffer; 
    
    //cycle, (addr, val)
    std::list<std::pair<std::pair<int,int>,std::pair<int,int> > > writes;
    //List to track register writes+their times

    SDL_AudioDeviceID id;
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;

    const static uint8_t SQ1 = 0;
    const static uint8_t SQ2 = 1;
    const static uint8_t TRI = 2;
    const static uint8_t NOISE = 3;
    const static uint8_t DMC = 4;

    static uint16_t noise_freq_table[16];

    static uint8_t pal_len_table[8];
    static uint8_t ntsc_len_table[8];
    static uint8_t general_len_table[16];

    const static uint8_t TRI_LEN = 32;
    static uint8_t TRI_STATES[TRI_LEN];
    const static uint8_t SQ_LEN = 16;
    static uint8_t SQ_STATES[4][SQ_LEN];
    static uint16_t NOISE_LEN[2];
    static uint8_t noise_states[2][32767];

    //Internal APU state registers
    status_reg stat;

    //4015
    bool enabled[5];

    //4000/4004/400C
    unsigned int vol[5]; //Volume if decay is disabled, decay rate, otherwise
                //The calculated decrement rate is 240Hz/(N+1)
    unsigned int decay_count[5]; //How many 240Hz cycles until decay the vol
    bool enable_decay[5]; //NES has "disable decay", but it's easier to think of the opposite, IMO
    bool loop_decay[5]; //Should the decay level loop?
    unsigned int decay_vol_lvl[5]; //current volume, if decay enabled. Decrements at rate "vol" when decay is on

    unsigned int duty_cycle[5]; //square register 4000/4004, noise register 400E

    //4008
    unsigned int linear_cntr; //in quarter-frames
    bool enable_linear_cntr; //Only really applies to triangle

    //4001/4005
    bool sweep_enable[5];
    unsigned int sweep_shift[5];
    unsigned int sweep_dir[5];
    unsigned int sweep_rate[5];
    unsigned int sweep_count[5];
/* 4001h - APU Sweep Channel 1 (Rectangle)
   4005h - APU Sweep Channel 2 (Rectangle)
  3       Sweep Direction          (0=[+]Increase, 1=[-]Decrease)
  4-6     Sweep update rate        (N=0..7), NTSC=120Hz/(N+1), PAL=96Hz/(N+1)
  (Ie. in Decrease mode: Channel 1 uses NOT, Channel 2 uses NEG)
Wavelength register will be updated only if all 3 of these conditions are met:
  Bit 7 is set (sweeping enabled)
  The shift value (which is S in the formula) does not equal to 0
  The channel's length counter contains a non-zero value
Sweep end: The channel gets silenced, and sweep clock is halted, when:
  1) current 11bit wavelength value is less than 008h
  2) new 11bit wavelength would become greater than 7FFh
Note that these conditions pertain regardless of any sweep refresh rate values, or if sweeping is enabled/disabled (via Bit7).
*/

    //4003/4007/400B/400F
    int length[5]; //in frames
    int wavelength_count_reset[5]; //max wave_pos value to reset to, in samples


    int wavelength_count[5]; //how long until clocking the waveform generator again, in samples
    int wave_pos[5]; //Current duty-cycle element (contains shift register value for noise generation)

    int pin1_enabled; //How many channels turned on for pin1 of the output (the two square channels)
    int pin2_enabled; //How many channels turned on for pin2 of the output (tri, noise, dmc)

    bool noise_flip_flop; //Used to half the speed that I clock samples

    int framecounter_pos; //Current position of the framecounter
    int framecounter_reset; //Value that the framecounter resets to 0 at
    bool enable_irq; //Is IRQ enabled
    int counter_mode;


    //4010, 4011, 4012, 4013, 4015
    int dmc_freq_table[16]; //Actually wavelengths, in terms of clock cycles to pass between clocking the DMC hardware for the next bit of data
    int dmc_pcm_data;       //Current value for PCM data coming from the DMC's DAC
    int dmc_addr;           //Address for sfx data
    int dmc_addr_reset;     //Address to reset to, for looping
    int dmc_freq_cnt;       //Wavelength counter
    int dmc_freq;           //Reset value for the wavelength counter
    bool dmc_gen_irq;       //Generate IRQ when the sound ends? (not implemented, but the emulator prints a warning about it)
    bool dmc_loop;          //Loop the effect when it ends? (not tested, but should work ;-) )
    int dmc_length;         //Byte length of the sfx
    int dmc_bit;            //Current bit selected for an increment value
    int dmc_cur_byte;       //Place to store the current byte when I fetch it from the CPU memory map

    int frame;              //Keeps track of the current video frame so that I process register writes at the right time
};

