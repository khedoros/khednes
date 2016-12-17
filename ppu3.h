#pragma once
#include "rom.h"
#include "util.h"
#include "SDL2/SDL.h"
#include <list>
#include "compositor.h"

class ppu {
private:
        //Used to describe a PPU read/write for later processing
        class log_entry {
            int cycle;
            int address;
            int val;
            public:
            log_entry(int cycle,int address,int val) : cycle(cycle), address(address), val(val) {}
        };

        //Stats storage, just for curiosity's sake
        class stats {
            public:
            stats() : reads(0), twok2_reads(0), reads_during_render(0), writes_during_render(0), dmas(0), writes(0), frames(0), cycles(0), pixels(0) {}
            unsigned int reads;
            unsigned int twok2_reads;
            unsigned int reads_during_render;
            unsigned int writes_during_render;
            unsigned int dmas;
            unsigned int writes;
            unsigned int frames;
            uint64_t cycles;
            unsigned int pixels;
        };

        //0x2000 PPU Control Register 1 (WO)
        typedef union {
            unsigned char reg;
            struct {
                unsigned base_addr:2; //base nametable address (00=$2000, 01=$2400, 02=$2800, 03=$2c00
                unsigned v_inc_down:1; //vram pointer increment mode (0=by 1, 1=by 32)
                unsigned sprite_table:1; //Sprite pattern table selection (0=$0000, 1=$1000)
                unsigned bg_table:1; //BG pattern table selection (0=$0000, 1=$1000)
                unsigned big_sprites:1; //select 8x8 or 8x16 sprites (0=8x8, 1=8x16)
                unsigned ppu_layer:1; //should ALWAYS be zero!!
                unsigned generate_vblank:1; //VBlank generation enabled. (0=disabled, 1=enabled)
            };
        } ppu_control;

        //0x2001 PPU Control Register 2 (WO)
        typedef union {
            unsigned char reg;
            struct {
                unsigned color_disabled:1; //monochrome display? (0=color behaves normally, 1=AND all palette entries with 110000)
                unsigned show_left_bg:1; //Show leftmost 8 pixels of bg? (0=false, 1=true)
                unsigned show_left_sprites:1; //Show sprites in leftmost 8 pixels? (0=false, 1=true)
                unsigned show_bg:1; //Enable background? (0=false, 1=true)
                unsigned show_sprites:1; //Enable sprites? (0=false, 1=true)
                unsigned green_intensify:1;
                unsigned blue_intensify:1;
                unsigned red_intensify:1;
            } bits;
        } ppu_mask;

        //0x2002 PPU Status Register (RO)
        typedef union {
            unsigned char reg;
            struct {
                unsigned low_bits:5; //Lowest 5 bits of last ppu register write
                unsigned sprite_overflow:1; //Over 8 sprites shown on this line already?
                unsigned sprite_0:1; //Has sprite 0 collided with the background?
                unsigned vsync:1; //Is PPU currently in vsync?
            } bits;
        } ppu_status;

        //VRAM pointer, programmed by 0x2006 and 0x2005
        typedef union {
            struct {
                unsigned tile_x:5;
                unsigned tile_y:5;
                unsigned page_x:1;
                unsigned page_y:1;
                unsigned fine_y:3;
            } fields;
            struct {
                unsigned low:8;
                unsigned high:6;
                unsigned pad:2;
            } bytes;
            unsigned pointer:14; //0->0x3FFF or 0->16383
            unsigned nt_pointer:12; //0->FFF or 0->4095
            unsigned reg:16; //0->0xFFFF or 0->65535
        } vram_pointer;

        //Byte type, with a lot of useful bit masks
        typedef union {
            unsigned char val; //Used to set the value
            struct {
                unsigned low:2;
                unsigned high:6;
            } two_six; //Used for page selection from 0x2000
            struct {
                unsigned low:6;
                unsigned high:2;
            } six_two; //Used to mask off the top 2 bits for second write to 2006?
            struct {
                unsigned low:3;
                unsigned high:5;
            } three_five;
            struct {
                unsigned low:5;
                unsigned high:3;
            } five_three;
            struct {
                unsigned bit0:1;//0x01
                unsigned bit1:1;//0x02
                unsigned bit2:1;//0x04
                unsigned bit3:1;//0x08
                unsigned bit4:1;//0x10
                unsigned bit5:1;//0x20
                unsigned bit6:1;//0x40
                unsigned bit7:1;//0x80
            } bits; //individual bits
            struct {
                unsigned first:2;
                unsigned second:2;
                unsigned third:2;
                unsigned fourth:2;
            } pairs; //Used for attrib table access
        } byte;

        typedef union {
            unsigned char val[4];
            struct {
                unsigned y:8;
                unsigned tile:8;

                unsigned pal:2;
                unsigned unused:3;
                unsigned priority:1;
                unsigned hflip:1;
                unsigned vflip:1;

                unsigned x:8;
            };
        } oam_data;

        unsigned int s0_hit_cycle(unsigned int cycle);
        unsigned int overflow_cycle(unsigned int cycle);
        void set_mirror(unsigned int);
        void clear_surfaces(bool bank);
        static const std::string phases[6];

        rom& cart;
        compositor screen;
        stats s;

        SDL_Surface* get_Surface(int, int, int, int, int);
        void unget_Surface(int, int, int);
         
        void blit_sprite(int);
        int get_nt_pal(int, int, int);

        void set_scroll(int x, int y); //Set where the picture starts rendering
        void set_scroll(int rx, int ry, int mod_col, int mod_row); //Set where X-scroll is modified mid-frame
        void apply_updates_to(int cycle);
        int init_scroll_x;
        int init_scroll_y;
        std::list<std::pair<int, int> > scroll_shifts;

        oam_data sprites[64];
        unsigned char spraddr;
        int sprites_on_line[240];
        unsigned int spr_overflow_cycle;
        unsigned int spr_hit0_cycle;
        bool recalc_overflow;
        bool recalc_s0;

        vram_pointer vram_ptr_reset;
        vram_pointer vram_ptr;
        unsigned fine_x:3;
        unsigned char ppu_ram_buffer;
        bool scroll_latch;
        bool cleared_vsync;
        bool late_vsync_clear;
        unsigned int cycles_per_frame;
        unsigned char name_table[0x1000];
        unsigned char * nt[4];
        unsigned char * at[4];

        unsigned char palette[32];
        unsigned char * pal[32];

        //int xscroll;
        //int yscroll;

        //Stores up to 16384 tile variants, but only when the exact combination is in use
        //tile_cache[table#][tile#][pal#][hflip][vflip]
        SDL_Surface * tile_cache[2][256][8][2][2];

        ppu_control control0; //0x2000
        ppu_mask control1; //0x2001
        ppu_status status; //0x2002


        int frame;

        bool table_view;

public:
        ppu(rom& romi, int res);
        ~ppu();

        typedef enum {
            DUNNO,
            VINT,
            PRE_RENDER,
            RENDER,
            POST_RENDER,
            DISABLED
        } ppu_phase;

        const ppu_phase phase(const unsigned int cycle);
        const unsigned int get_ppu_cycles(const unsigned int frame);
        void dma(unsigned char val, unsigned int cycle);
        void reg_write(const unsigned int addr,const unsigned char val, const unsigned int cycle);
        unsigned char reg_read(const unsigned int addr, const unsigned int cycle);
        int calc();
        int get_color(int tile, int x, int y);
        int get_frame();
        int get_s0_est();
        int get_vsync_start();
        int get_sprite_overflow_est();
        void toggle_table_view();
        void resize(int x, int y);
        void speedup(bool up);
        uint32_t get_buffer_color(int x, int y);
        void print_name_table();

};
