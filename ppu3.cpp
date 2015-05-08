#include "ppu3.h"
#include "nes_constants.h"
#include <assert.h>
#include <list>
#include <iostream>

#define ORIGIN 4

ppu::ppu(rom& romi, int res) : cart(romi), screen(1), scroll_shifts(0), frame(0) {
    //cout<<"Start ppu constructor"<<endl;
    //cout<<"\tRes: "<<res<<endl;
    //cout<<"End ppu constructor"<<endl<<endl;
    s.cycles = 0;
    s.reads = 0;
    s.twok2_reads = 0;
    s.writes = 0;
    s.dmas = 0;
    s.reads_during_render = 0;
    s.writes_during_render = 0;
    frame = 0;
    cleared_vsync = false;
    late_vsync_clear = false;
    spr_overflow_cycle = -1;
    spr_hit0_cycle = -1;
    recalc_overflow = true;
    recalc_s0 = true;
    table_view = false;
    init_scroll_x = init_scroll_y = 0;
    cycles_per_frame = CLK_PER_LINE * LINE_PER_FRAME;

    set_mirror(cart.get_mode());

    for(int i=0; i < 0x20; ++i)
        pal[i] = &(palette[i]);
    pal[0x10] = &(palette[0]);
    pal[0x14] = &(palette[4]);
    pal[0x18] = &(palette[8]);
    pal[0x1C] = &(palette[0x0c]);

    //tile_cache[table#][tile#][pal#][hflip][vflip]
    //        SDL_Surface * tile_cache[2][256][8][2][2];
    for(int table = 0; table < 2; ++table) {
        for(int tile = 0; tile < 256; ++tile) {
            for(int palette = 0; palette < 8; ++palette) {
                for(int hflip = 0; hflip < 2; ++hflip) {
                    for(int vflip = 0; vflip < 2; ++vflip) {
                        tile_cache[table][tile][palette][hflip][vflip] = NULL;
                    }
                }
            }
        }
    }
}

void ppu::set_mirror(unsigned int mode) {
    if(mode == VERT) {
        nt[0] = &(name_table[NT0_OFFSET]);
        nt[2] = nt[0];

        at[0] = &(name_table[AT0_OFFSET]);
        at[2] = at[0];

        nt[1] = &(name_table[NT1_OFFSET]);
        nt[3] = nt[1];

        at[1] = &(name_table[AT1_OFFSET]);
        at[3] = at[1];
    }
    else if(mode == HORIZ) {
        nt[0] = &(name_table[NT0_OFFSET]);
        nt[1] = nt[0];

        at[0] = &(name_table[AT0_OFFSET]);
        at[1] = at[0];

        nt[2] = &(name_table[NT1_OFFSET]);
        nt[3] = nt[2];

        at[2] = &(name_table[AT1_OFFSET]);
        at[3] = at[2];
    }
    else if(mode == FOUR_SCR) {
        nt[0] = &(name_table[NT0_OFFSET]);
        at[0] = &(name_table[AT0_OFFSET]);

        nt[1] = &(name_table[NT1_OFFSET]);
        at[1] = &(name_table[AT1_OFFSET]);

        nt[2] = &(name_table[NT2_OFFSET]);
        at[2] = &(name_table[AT2_OFFSET]);

        nt[3] = &(name_table[NT3_OFFSET]);
        at[3] = &(name_table[AT3_OFFSET]);
    }
    else { cout<<"WTF? Unrecognized mirror mode."<<endl;}
}

void ppu::clear_surfaces(bool bank) {
    if(!bank)
        for(int tile = 0; tile < 256; ++tile)
            for(int palette = 0; palette < 8; ++palette)
                unget_Surface(0,tile,palette);
    else
        for(int tile = 0; tile < 256; ++tile)
            for(int palette = 0; palette < 8; ++palette)
                unget_Surface(1,tile,palette);
}

const std::string ppu::phases[] = {std::string("DUNNO"),
                                   std::string("VINT"),
                                   std::string("PRE-RENDER"),
                                   std::string("RENDER"),
                                   std::string("POST-RENDER"),
                                   std::string("DISABLED")};

ppu::~ppu() {
    //cout<<"Start ppu desctructor"<<endl;
    cout<<"Reads: "<<s.reads<<" ("<<s.twok2_reads<<" to 0x2002, "<<s.reads_during_render<<" during render) Writes: "<<s.writes<<" ("<<s.writes_during_render<<" during render) DMAs: "<<(s.dmas/256)<<endl<<"Total cycles: "<<s.cycles<<" Total frames: "<<frame<<endl;
    clear_surfaces(false);
    clear_surfaces(true);
    while(scroll_shifts.size() > 0) scroll_shifts.pop_front();
    //cout<<"End ppu destructor"<<endl<<endl;
}

void ppu::dma(unsigned char val, unsigned int cycle) {
    //cout<<"DMA from 0x"<<hex<<val<<dec<<"00"<<endl;
    reg_write(0x2004, val, cycle);
    ++s.dmas;
}

const ppu::ppu_phase ppu::phase(const unsigned int cycle) {
    int odd_diff = ((frame % 2 == 0)?0:1);
    ppu_phase choice = DUNNO;

    if(!control1.bits.show_bg && !control1.bits.show_sprites) choice = DISABLED; //DISABLED
    else if(cycle < (unsigned int)(VINT_LINES * CLK_PER_LINE)) choice = VINT; //VINT
    else if(cycle < (unsigned int)((VINT_LINES + PRE_RENDER_LINES) * CLK_PER_LINE - odd_diff)) choice = PRE_RENDER; //PRE_RENDER
    else if(cycle < (unsigned int)((VINT_LINES + PRE_RENDER_LINES + RENDER_LINES) * CLK_PER_LINE) - odd_diff) choice = RENDER; //RENDER
    else if(cycle < (unsigned int)((VINT_LINES + PRE_RENDER_LINES + RENDER_LINES + POST_RENDER_LINES) * CLK_PER_LINE) - odd_diff) choice = POST_RENDER; //POST_RENDER
    return choice;
}

unsigned int ppu::s0_hit_cycle(unsigned int cycle) {
    //TODO: This is an inexact estimate. It needs fixed, sometime.
    //      (Better than the last version of the estimate, though ;-)
    if(recalc_s0) {
        if(sprites[0].y + 1 > 239) spr_hit0_cycle = -1;
        else {
            int spry = sprites[0].y + 1;
            int sprx = sprites[0].x;
            bool hf = sprites[0].hflip;
            bool vf = sprites[0].vflip;
            int palnum = sprites[0].pal | 4;
            int tile = sprites[0].tile;
            int table = control0.sprite_table;
            SDL_Surface * s = get_Surface(table, tile, palnum, hf, vf);
            int px = -1;
            int py = -1;
            for(int x = 0; x < 8; ++x)
                for(int y = 0; y < 8; ++y) {
                    unsigned char t =((unsigned char *)s->pixels)[y*(s->pitch)+x];
                    if(t != NES_TRANSPARENT) {
                        px = x; py = y; /*cout<<"x: "<<x<<" y: "<<y<<" val: "<<int(t)<<endl;*/ goto loop_exit;
                    }
                }
            loop_exit:
            if(px == -1) spr_hit0_cycle = -1;
            else spr_hit0_cycle = ((spry + 1 + py) * CLK_PER_LINE + sprx + px);
            recalc_s0 = false;
        }
    }
    return spr_hit0_cycle;
}

unsigned int ppu::overflow_cycle(unsigned int cycle) {
    if(recalc_overflow) {
        int lines[240] = {0};
        spr_overflow_cycle = 0;
        for(int sprite = 0; sprite < 64; ++sprite) { //For each sprite...
            //Increment sprite count for each line that the sprite is visible on
            for(int y = sprites[sprite].y; (y < sprites[sprite].y + 8 + (control0.big_sprites * 8)) && y < 240; ++y)
                ++lines[y];
        }
        for(int line = 0; line < 240; ++line) {
            if(lines[line] > 8) {
                spr_overflow_cycle = line * CLK_PER_LINE;
                break;
            }
        }
        //if no overflow found, set the overflow value out of range (ie no overflow occurs)
        if(spr_overflow_cycle == 0) spr_overflow_cycle = -1;
        recalc_overflow = false;
    }
    return spr_overflow_cycle;
}

void ppu::reg_write(const unsigned int addr,const unsigned char val, const unsigned int cycle) {
    //cout<<"!! "<<phases[phase(cycle)]<<" Write address: 0x"<<hex<<addr<<" Value: 0x"<<int(val)<<dec<<" Cycle: "<<cycle<<endl;
    ppu_phase curphase = phase(cycle);
    if(curphase == RENDER) ++s.writes_during_render;
    switch(addr) {
        case 0x2000:
        {
            int bg_table = control0.bg_table;
            int spr_table = control0.sprite_table;
            control0.reg = val;
            vram_ptr_reset.fields.page_y = (control0.base_addr & 2)/2;
            vram_ptr_reset.fields.page_x = (control0.base_addr & 1);

            int xscroll = (control0.base_addr%2) * 32 + vram_ptr_reset.fields.tile_x;
            int yscroll = (control0.base_addr/2) * 30 + vram_ptr_reset.fields.tile_y;
            if(curphase != RENDER && curphase != POST_RENDER) { //keeps track of where the screen will start
                set_scroll(xscroll*8+fine_x, yscroll*8+vram_ptr_reset.fields.fine_y);
                cart.ppu_change(cycle, BG_CHANGE_ADDR, control0.bg_table);
                cart.ppu_change(cycle, SPR_CHANGE_ADDR, control0.sprite_table);
                //if(control0.bg_table != bg_table) cout<<"Changed the bg table (to "<<control0.bg_table<<"), cycle: "<<cycle<<endl;
                //if(control0.sprite_table != spr_table) cout<<"Changed the sprite table (to "<<control0.sprite_table<<"), cycle: "<<cycle<<endl;
            } else { //keeps track of when (during rendering) the reset shifts, but only worry about the X reset
                set_scroll(xscroll*8+fine_x, yscroll*8+vram_ptr_reset.fields.fine_y, cycle%CLK_PER_LINE, cycle/CLK_PER_LINE);
                if(control0.bg_table != bg_table) {
                    //cout<<"Changed the bg table mid-frame! (to "<<control0.bg_table<<"), cycle: "<<cycle<<endl;
                    cart.ppu_change(cycle, BG_CHANGE_ADDR, 1 - control0.bg_table);
                }
                if(control0.sprite_table != spr_table) {
                    //cout<<"Changed the sprite table mid-frame! (to "<<control0.sprite_table<<"), cycle: "<<cycle<<endl;
                    cart.ppu_change(cycle, SPR_CHANGE_ADDR, 1 - control0.sprite_table);
                }
            }
        }
            break;
        case 0x2001:
            /*
            cout<<"Wrote "<<hex<<val<<dec<<" to 2001 (Color disabled: "<<control1.bits.color_disabled
                <<" show left bg: "<<control1.bits.show_left_bg<<" show left sprites: "<<control1.bits.show_left_sprites
                <<" show bg: "<<control1.bits.show_bg<<" show sprites: "<<control1.bits.show_sprites<<endl;
            */
            control1.reg = val;
            break;
        case 0x2003:
            spraddr = val;
            break;
        case 0x2004:
            if(spraddr/4 == 0) recalc_s0 = true;
            recalc_overflow = true;
            sprites[spraddr/4].val[spraddr%4] = val;
            spraddr++;
            break;
        case 0x2005:
            if(!scroll_latch) {
                vram_ptr_reset.fields.tile_x = (val & 0xf8)>>(3) ;
                fine_x = (val & 0x7);
                //cout<<"Set X reset to ("<<(control0.base_addr&1)*256+vram_ptr_reset.fields.tile_x*8+fine_x<<", "
                //    <<(control0.base_addr&2)*120+vram_ptr_reset.fields.tile_y*8+vram_ptr_reset.fields.fine_y<<") at cycle "
                //    <<cycle<<" ("<<cycle/CLK_PER_LINE<<", "<<cycle%CLK_PER_LINE<<") ("<<phases[curphase]<<")"<<endl;
            }
            else {
                vram_ptr_reset.fields.tile_y = (val & 0xf8)>>(3);
                vram_ptr_reset.fields.fine_y = (val & 0x7);
                //cout<<"Set Y reset to ("<<(control0.base_addr&1)*256+vram_ptr_reset.fields.tile_x*8+fine_x<<", "
                //    <<(control0.base_addr&2)*120+vram_ptr_reset.fields.tile_y*8+vram_ptr_reset.fields.fine_y<<") at cycle "
                //    <<cycle<<" ("<<cycle/CLK_PER_LINE<<", "<<cycle%CLK_PER_LINE<<") ("<<phases[curphase]<<")"<<endl;
            }
            if(curphase != RENDER) { //keeps track of where the screen will start
                int xscroll = (control0.base_addr%2) * 32 + vram_ptr_reset.fields.tile_x;
                int yscroll = (control0.base_addr/2) * 30 + vram_ptr_reset.fields.tile_y;
                set_scroll(xscroll*8+fine_x, yscroll*8+vram_ptr_reset.fields.fine_y);
            }
            else if(curphase == RENDER) { //keeps track of when (during rendering) the reset shifts, but only worry about the X reset
                int xscroll = (control0.base_addr%2) * 32 + vram_ptr_reset.fields.tile_x;
                int yscroll = (control0.base_addr/2) * 30 + vram_ptr_reset.fields.tile_y;
                set_scroll(xscroll*8+fine_x, yscroll*8+vram_ptr_reset.fields.fine_y, cycle%CLK_PER_LINE, cycle/CLK_PER_LINE);
            }
            scroll_latch = !scroll_latch;
            break;
        case 0x2006:
            if(!scroll_latch) {
                vram_ptr_reset.bytes.high = val;
                vram_ptr_reset.bytes.pad = 0;
            }
            else {
                vram_ptr_reset.bytes.low = val;
                vram_ptr = vram_ptr_reset;
            }

            if(curphase != RENDER) { //keep track of where the screen starts
                int xscroll = (control0.base_addr%2) * 32 + vram_ptr_reset.fields.tile_x;
                int yscroll = (control0.base_addr/2) * 30 + vram_ptr_reset.fields.tile_y;
                //cout<<"2006: set scroll to ("<<yscroll<<", "<<xscroll<<")"<<endl;
                set_scroll(xscroll*8+fine_x, yscroll*8+vram_ptr_reset.fields.fine_y);
            }
            else if(curphase == RENDER) {
                int xscroll = (control0.base_addr%2) * 32 + vram_ptr_reset.fields.tile_x;
                int yscroll = (control0.base_addr/2) * 30 + vram_ptr_reset.fields.tile_y;
                set_scroll(xscroll*8+fine_x, yscroll*8+vram_ptr_reset.fields.fine_y, cycle%CLK_PER_LINE, cycle/CLK_PER_LINE);
            }
            scroll_latch = !scroll_latch;
            break;
        case 0x2007:
            //cout<<"0x2007 (0x"<<hex<<vram_ptr.pointer<<dec<<"): "<<hex<<int(val)<<dec<<"(";
            if(vram_ptr.pointer < 0x2000) {
                //cout<<"Writing to Pattern Table)"<<endl;
                bool wrote = cart.put_cbyte(val, vram_ptr.pointer);
                if(wrote) {
                    int table = (vram_ptr.pointer < 0x1000)?0:1;
                    int tile = (vram_ptr.pointer & 0xfff) / 16;
                    for(int palnum = 0; palnum < 8; ++palnum) {
                        unget_Surface(table, tile, palnum);
                    }
                }
            }
            else {
                unsigned int ptr = vram_ptr.pointer;
                if(ptr >= 0x3000 && ptr < 0x3F00) ptr -= 0x1000;
                if(ptr < 0x23C0) {
                    //cout<<"Writing to Name Table 0)"<<endl;
                    nt[0][ptr - 0x2000] = val;
                }
                else if(ptr < 0x2400) {
                    //cout<<"Writing to Attribute Table 0)"<<endl;
                    at[0][ptr - 0x23C0] = val;
                }
                else if(ptr < 0x27C0) {
                    //cout<<"Writing to Name Table 1)"<<endl;
                    nt[1][ptr - 0x2400] = val;
                }
                else if(ptr < 0x2800) {
                    //cout<<"Writing to Attribute Table 1)"<<endl;
                    at[1][ptr - 0x27C0] = val;
                }
                else if(ptr < 0x2BC0) {
                    //cout<<"Writing to Name Table 2)"<<endl;
                    nt[2][ptr - 0x2800] = val;
                }
                else if(ptr < 0x2C00) {
                    //cout<<"Writing to Attribute Table 2)"<<endl;
                    at[2][ptr - 0x2BC0] = val;
                }
                else if(ptr < 0x2FC0) {
                    //cout<<"Writing to Name Table 3)"<<endl;
                    nt[3][ptr - 0x2C00] = val;
                }
                else if(ptr < 0x3000) {
                    //cout<<"Writing to Attribute Table 3)"<<endl;
                    at[3][ptr - 0x2FC0] = val;
                }
                else {
                    ptr &= 0x1f;
                    if(ptr < 0x10) {
                        //cout<<"Writing to Background Palette)"<<endl;
                    }
                    else {
                        //cout<<"Writing to Sprite Palette)"<<endl;
                    }
                    int oldval = *(pal[ptr]);
                    if(val != oldval) {
                        *(pal[ptr]) = (val&0x3f);
                        //cout<<hex<<"Palette value "<<ptr<<" changed, clearing palette "<<(ptr>>(2))<<" tiles"<<endl;
                        for(int table = 0; table < 2; ++table) {
                            for(int tile = 0; tile < 256; ++tile) {
                                unget_Surface(table, tile, ptr>>(2));
                            }
                        }
                    }
                }
            }
            vram_ptr.pointer++;
            if(control0.v_inc_down) vram_ptr.pointer +=31;
            break;
        default:
            cout<<"Writing to register 0x"<<hex<<addr<<" isn't handled."<<dec<<endl;
        break;
    }
    ++s.writes;
}

unsigned char ppu::reg_read(const unsigned int addr, unsigned int cycle) {
    //cout<<"!! "<<phases[phase(cycle)]<<" Read address: 0x"<<hex<<addr<<dec<<" Cycle: "<<cycle<<endl;
    if(phase(cycle) == RENDER) ++s.reads_during_render;
    unsigned char retval = 0;
    switch(addr) {
    case 0x2002:
        if(!cleared_vsync && (phase(cycle) == VINT || cycle >= cycles_per_frame - 85)) {
            retval |= 0x80;
            cleared_vsync = true;
            //TODO: Reduce reliance on hard-coded cycle counts
            if(cycle >= cycles_per_frame - 85) late_vsync_clear = true;
        }
        if(s0_hit_cycle(cycle) <= cycle) {
            retval |= 0x40;
        }
        if(overflow_cycle(cycle) <= cycle) {
            retval |= 0x20;
        }
        scroll_latch = false;
        ++s.twok2_reads;
        break;
    case 0x2004:
        retval = sprites[spraddr/4].val[spraddr%4];
        break;
    case 0x2007:
        //cout<<"0x2007 (0x"<<hex<<vram_ptr.pointer<<dec<<"): (";
        if(vram_ptr.pointer < 0x2000) {
            //cout<<"Reading from Pattern Table)";
            retval = ppu_ram_buffer;
            ppu_ram_buffer = cart.get_cbyte(vram_ptr.pointer);
        }
        else {
            unsigned int ptr = vram_ptr.pointer;
            retval = ppu_ram_buffer;
            if(ptr >= 0x3000 && ptr < 0x3F00) ptr -= 0x1000;
            if(ptr < 0x23C0) {
                //cout<<"Reading from Name Table 0)";
                ppu_ram_buffer = nt[0][ptr - 0x2000];
            }
            else if(ptr < 0x2400) {
                //cout<<"Reading from Attribute Table 0)";
                ppu_ram_buffer = at[0][ptr - 0x23C0];
            }
            else if(ptr < 0x27C0) {
                //cout<<"Reading from Name Table 1)";
                ppu_ram_buffer = nt[1][ptr - 0x2400];
            }
            else if(ptr < 0x2800) {
                //cout<<"Reading from Attribute Table 1)";
                ppu_ram_buffer = at[1][ptr - 0x27C0];
            }
            else if(ptr < 0x2BC0) {
                //cout<<"Reading from Name Table 2)";
                ppu_ram_buffer = nt[2][ptr - 0x2800];
            }
            else if(ptr < 0x2C00) {
                //cout<<"Reading from Attribute Table 2)";
                ppu_ram_buffer = at[2][ptr - 0x2BC0];
            }
            else if(ptr < 0x2FC0) {
                //cout<<"Reading from Name Table 3)";
                ppu_ram_buffer = nt[3][ptr - 0x2C00];
            }
            else if(ptr < 0x3000) {
                //cout<<"Reading from Attribute Table 3)";
                ppu_ram_buffer = at[3][ptr - 0x2FC0];
            }
            else {
                ptr &= 0x1f;
                if(ptr < 0x10) {
                    //cout<<"Reading from Background Palette)";
                }
                else {
                    //cout<<"Reading from Sprite Palette)";
                }
                retval = *(pal[ptr]);
                ppu_ram_buffer = retval;
            }
        }
        //cout<<hex<<": 0x"<<int(retval)<<dec<<endl;
        vram_ptr.pointer++;
        if(control0.v_inc_down) vram_ptr.pointer +=31;
        break;
    default:
        cout<<"Reading from register 0x"<<hex<<addr<<" isn't handled."<<dec<<endl;
        break;
    }
    ++s.reads;
    return retval;
}

int ppu::calc() {

    int crom_update = cart.changed_crom();
    if((crom_update&1) == 1) clear_surfaces(false);
    if((crom_update&2) == 2) clear_surfaces(true);

    //cout<<"Total cycles: "<<s.cycles<<" Total frames: "<<frame<<endl;
    s.cycles += cycles_per_frame;
    ++frame;

    if(!late_vsync_clear) {
        cleared_vsync = false;
    }
    else late_vsync_clear = false;

    if(frame % 2 == 0) cycles_per_frame = CLK_PER_LINE * LINE_PER_FRAME;
    else cycles_per_frame = CLK_PER_LINE * LINE_PER_FRAME - 1;

    if(table_view) {
        //Draw 64 sprites, always, since I can change the blit location easily in compositor::flip()
        for(int spr=63;spr>=0;--spr) {
            blit_sprite(spr);           
        }
        //Draw the palette
        for(int palnum = 0; palnum < 16; ++palnum) {
            for(int x=palnum*16; x < palnum*16 + 16; ++x)
                for(int y=0; y < 16; ++y) {
                    screen.pset(x,y+480,*pal[palnum]);
                    screen.pset(x,y+496,*pal[palnum+16]);
                }
        }
        //Draw the state of the name table
        for(int table = 0; table < 4; ++table) {
            for(int tile_x = 0; tile_x < 32; ++tile_x) {
                for(int tile_y = 0; tile_y < 30; ++tile_y) {
                    int val = nt[table][tile_y * 32 + tile_x];
                    int pal = get_nt_pal(table, tile_x, tile_y);
                    //screen.erase_nt(x,y);
                    int x_offset = 0;
                    int y_offset = 0;
                    if(table == 1)
                        x_offset = 256;
                    else if(table == 2)
                        y_offset = 240;
                    else if(table == 3) {
                        y_offset=240;
                        x_offset=256;
                    }
                    screen.blit_nt(get_Surface(control0.bg_table, val, pal, 0,0), tile_x*8+x_offset, tile_y*8+y_offset);
                }
            }
        }
    }
    else if(control1.bits.show_bg) { //Normal game render

        int xspix = init_scroll_x;
        int yspix = init_scroll_y;

        int xstile = xspix / 8;
        int ystile = yspix / 8;

        int xepix = xspix + 256;
        int xetile = xepix / 8 + 1;

        int yepix = 0;
        //int yetile = 0;

        int curytile = 0;

        unsigned int odd_diff = ((frame % 2 == 0)?0:1);
        bool process = true;
        while(process) {
            if(scroll_shifts.size() > 0) {
                yepix = scroll_shifts.front().second + init_scroll_y;
            }
            else {
                yepix = init_scroll_y + 240;
                process = false;
            }
            int yetile = yepix / 8 + 1;

            for(int yt = ystile; yt < yetile; ++yt) {
                unsigned int curcycle = (unsigned int)((VINT_LINES + PRE_RENDER_LINES) * CLK_PER_LINE - odd_diff) + curytile * 8 * CLK_PER_LINE;
                apply_updates_to(curcycle);

                //Draw sprites that should show by now
                if(control1.bits.show_sprites)
                    for(int i=0;i<64;++i)
                        if(sprites[i].y <= curytile * 8)
                            blit_sprite(i);

                //Draw this line of tiles
                for(int xt = xstile; xt < xetile; ++xt) {
                    int table = ((xt / 32) % 2) + (((yt / 30) % 2) * 2);
                    int tile_x = xt % 32;
                    int tile_y = yt % 30;
                    int pal = get_nt_pal(table, tile_x, tile_y);
                    int val = nt[table][tile_y * 32 + tile_x];
                    screen.blit_nt(get_Surface(control0.bg_table, val, pal, 0, 0), xt*8, yt*8);
                }
                ++curytile;
            }
            //Send that region to compositor to be rendered
            screen.add_nt_area(xspix, yspix, xepix, yepix);

            //Process the rest of the shifts, then delete the entry
            if(scroll_shifts.size() > 0) {
                xspix = scroll_shifts.front().first;
                xstile = xspix / 8;

                xepix = xspix + 256;
                xetile = xepix / 8 + 1;

                yspix = yepix;
                ystile = yetile - 1;
                scroll_shifts.pop_front();
            }
        }
        apply_updates_to(100000); //way past end of frame
    }

    if(control1.bits.show_bg || control1.bits.show_sprites) {
        //Set background color
        screen.fill(*pal[0]);
    }
    //Composite the image and display it
    screen.flip();
    if(control0.generate_vblank) {
        //cout<<"Returning 1 (NMI vblank)"<<endl;
        return 1;
    }
    else {
        //cout<<"Returning 0 (no NMI)"<<endl;
        return 0;
    }
}

int ppu::get_color(int tile, int x, int y) {
    if(y >= 8 || x >= 8 || tile >= 512) {
        cout<<"Tile: "<<tile<<" ("<<x<<", "<<y<<") doesn't exist."<<endl;
        return -1;
    }
    int address = tile * 16 + y;
    int byte1 = cart.get_cbyte(address);
    int byte2 = cart.get_cbyte(address + 8);
    return ((((byte1 & (128>>(x))) > 0)?1:0) + (((byte2 & (128>>(x))) > 0)?2:0));
}
 
SDL_Surface * ppu::get_Surface(int table, int tile, int palnum, int hf, int vf) {
    //cout<<"T: "<<table<<" t: "<<tile<<" pal: "<<palnum<<" hf: "<<hf<<" vf: "<<vf<<endl;
    if(!(tile_cache[table][tile][palnum][hf][vf])) {
        //build the tile (8x8 palettized)
        SDL_Surface * temp = SDL_CreateRGBSurface(0,8,8,8,0,0,0,0);
        SDL_SetSurfacePalette(temp, screen.get_palette());
        SDL_SetColorKey(temp, SDL_TRUE, NES_TRANSPARENT);
        bool needs_lock = SDL_MUSTLOCK(temp);
        if(needs_lock) {
            SDL_LockSurface(temp);
            cout<<"Surfaces need locked!"<<endl;
        }
        int tilenum = table * 256 + tile;
        for(int y = 0; y < 8; ++y) {
            int outy;
            if(vf) outy = 7 - y;
            else outy = y;
            for(int x = 0; x < 8; ++x) {
                int outx;
                if(hf) outx = 7 - x;
                else outx = x;
                int color = get_color(tilenum,x,y);
                if(color == 0) ((uint8_t *)(temp->pixels))[outy*(temp->pitch)+outx]=NES_TRANSPARENT; //Transparent
                else {
                    //cout<<hex<<"Palnum: "<<palnum<<" Colorpart: "<<color<<" Both: "<<((palnum<<(2))+color);
                    color = *(pal[(palnum<<(2)) + color]);
                    //cout<<" val: "<<color<<dec<<endl;
                    ((uint8_t *)(temp->pixels))[outy*(temp->pitch)+outx]=color;
                }
            }
        }
        if(needs_lock) SDL_UnlockSurface(temp);
        tile_cache[table][tile][palnum][hf][vf] = temp;
    }
   return tile_cache[table][tile][palnum][hf][vf];
}

void ppu::unget_Surface(int table, int tile, int palnum) {
    for(int hf = 0; hf < 2; ++hf) {
        for(int vf = 0; vf < 2; ++vf) {
            SDL_Surface * temp = tile_cache[table][tile][palnum][hf][vf];
            if(!temp) continue;
            SDL_FreeSurface(temp);
            tile_cache[table][tile][palnum][hf][vf] = NULL;
        }
    }
}

void ppu::blit_sprite(int spr) {
    if(spr < 0 || spr > 63) return;
    int spry = sprites[spr].y;
    int sprx = sprites[spr].x;
    bool priority = !sprites[spr].priority;
    bool hf = sprites[spr].hflip;
    bool vf = sprites[spr].vflip;
    int palnum = sprites[spr].pal | 4;
    int tile1 = sprites[spr].tile;
    int table = control0.sprite_table;
    int tile2 = -1;
    if(control0.big_sprites) {
        if(tile1%2 == 0)
            table = 0;
        else
            table = 1;
        tile1 &= 0xfe;
        tile2 = tile1 + 1;
    }
    if(priority) {
        if(control0.big_sprites) {
            if(vf) {
                screen.blit_hps(get_Surface(table,tile2,palnum,hf,vf), sprx,spry+1);
                screen.blit_hps(get_Surface(table,tile1,palnum,hf,vf), sprx,spry+9);
            }
            else {
                screen.blit_hps(get_Surface(table,tile1,palnum,hf,vf), sprx,spry+1);
                screen.blit_hps(get_Surface(table,tile2,palnum,hf,vf), sprx,spry+9);
            }
        }
        else {
            screen.blit_hps(get_Surface(table,tile1,palnum,hf,vf), sprx,spry+1);
        }
    }
    else {
        if(control0.big_sprites) {
            if(vf) {
                screen.blit_lps(get_Surface(table,tile2,palnum,hf,vf), sprx,spry+1);
                screen.blit_lps(get_Surface(table,tile1,palnum,hf,vf), sprx,spry+9);
            }
            else {
                screen.blit_lps(get_Surface(table,tile1,palnum,hf,vf), sprx,spry+1);
                screen.blit_lps(get_Surface(table,tile2,palnum,hf,vf), sprx,spry+9);
            }
        }
        else {
            screen.blit_lps(get_Surface(table,tile1,palnum,hf,vf), sprx,spry+1);
        }
    }
}

int ppu::get_nt_pal(int table, int tx, int ty) {
    byte at_val;
    at_val.val = at[table][(ty/4)*8+(tx/4)];
    if(tx % 4 < 2 && ty % 4 < 2) return at_val.pairs.first;
    else if(tx % 4 < 2) return at_val.pairs.third;
    else if(ty % 4 < 2) return at_val.pairs.second;
    else return at_val.pairs.fourth;
}

int ppu::get_frame() {
    //cout<<"Returning "<<frame<<" (framecount)"<<endl;
    return frame;
}

const unsigned int ppu::get_ppu_cycles(const unsigned int frame) {
    return cycles_per_frame;
}

int ppu::get_s0_est() {
    //cout<<"Returning S0 est: 0"<<endl;
    return 0;
}

void ppu::toggle_table_view() {
    table_view = !table_view;
    if(table_view)
        cout<<"Turn on table view"<<endl;
    else
        cout<<"Turn off table view"<<endl;
    screen.table_view(table_view);
    
}

void ppu::resize(int x, int y) {
    //cout<<"Resizing screen to ("<<x<<", "<<y<<")"<<endl;
    screen.resize(x,y);
}

void ppu::speedup(bool up) {
    screen.speedup(up);
}

//Set the (x,y) reset values for the start of a frame
void ppu::set_scroll(int x, int y) {
    init_scroll_x = x;
    init_scroll_y = y;
    //screen.set_scroll(x,y);
}

//Notify the PPU that it's going to need to change the scroll mid-render
//rx and ry are the new reset values, mod_col and mod_row describe when the change came in
void ppu::set_scroll(int rx, int ry, int mod_col, int mod_row) {
    if(scroll_shifts.size() == 0 && rx != init_scroll_x) {
        //cout<<"First: Shifting scroll to "<<rx<<" at line "<<mod_row<<", frame "<<frame<<" (isx="<<init_scroll_x<<", size="<<scroll_shifts.size()<<")"<<endl;
        scroll_shifts.push_back(std::make_pair(rx,mod_row));
    }
    else if(scroll_shifts.size() > 0 && rx != scroll_shifts.back().first) {
        //cout<<"C1: "<<(scroll_shifts.size() == 0)<<" C2: "<<(rx!=init_scroll_x)<<endl;
        //cout<<"Second: Shifting scroll to "<<rx<<" at line "<<mod_row<<", frame "<<frame<<" (isx="<<init_scroll_x<<", size="<<scroll_shifts.size()<<")"<<endl;
        scroll_shifts.push_back(std::make_pair(rx,mod_row));
    }
    else {
        //cout<<"Filtering this scroll."<<endl;
        return;
    }
}

void ppu::apply_updates_to(int curcycle) {
    //If there've been any mid-screen updates, apply them at the appropriate time (accurate to 1 vertical tile-ish
    rom::ppu_change_t changes = cart.cycle_forward(curcycle - (8 * 2* CLK_PER_LINE));
    if(changes.lo_crom == 1) clear_surfaces(false);
    if(changes.hi_crom == 1) clear_surfaces(true);
    if(changes.set_horiz == 1 || changes.set_vert == 1 || changes.set_4_scr == 1) set_mirror(cart.get_mode());
    if(changes.set_bg0 == 1) control0.bg_table = 0;
    if(changes.set_bg1 == 1) control0.bg_table = 1;
    if(changes.set_spr0 == 1) control0.sprite_table = 0;
    if(changes.set_spr1 == 1) control0.sprite_table = 1;
    int mode_count = changes.set_horiz + changes.set_vert + changes.set_4_scr;
    if(mode_count > 1) cout<<"Multiple mirroring modes set during one vertical tile of time!"<<endl; 
}
