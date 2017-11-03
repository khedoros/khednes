#include "compositor.h"
#include "nes_constants.h"

compositor::compositor(int res) {
    /* Initialize the SDL library */
    screen = SDL_CreateWindow("KhedNES",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              320, 240,
                              SDL_WINDOW_RESIZABLE);
    if ( screen == NULL ) {
        fprintf(stderr, "Couldn't set 320x240x8 video mode: %s\nStarting without video output.\n",
                SDL_GetError());
        //exit(1);
        dummy = true;
        return;
    }

    dummy = false;

    SDL_SetWindowMinimumSize(screen, 320, 240);
    cur_x_res=256;
    cur_y_res=240;

    render = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED/*|SDL_RENDERER_PRESENTVSYNC*/);
    //render = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    //render = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE|SDL_RENDERER_PRESENTVSYNC);
    //render = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE/*|SDL_RENDERER_PRESENTVSYNC*/);
    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,256,240);

    buffer = SDL_CreateRGBSurface(0,256,240,8,0,0,0,0);
    overlay = SDL_CreateRGBSurface(0,256,240,8,0,0,0,0);
    lps = SDL_CreateRGBSurface(0,256,240,8,0,0,0,0);
    hps = SDL_CreateRGBSurface(0,256,240,8,0,0,0,0);
    nt = SDL_CreateRGBSurface(0,768,720,8,0,0,0,0);

    SDL_SetRenderDrawColor(render, 0, 0, 0, 0);
    SDL_RenderClear(render);
    SDL_RenderPresent(render);

    for(int i=0;i<76;++i) {
        tv_palette[i].r=palette_table[i*3];
        tv_palette[i].g=palette_table[i*3+1];
        tv_palette[i].b=palette_table[i*3+2];
        if(i==NES_TRANSPARENT)
            tv_palette[i].a = 0;
        else
            tv_palette[i].a=255;
    }

    palstruct = SDL_AllocPalette(256);
    SDL_SetPaletteColors(palstruct,&tv_palette[0],0,76);

    SDL_SetSurfacePalette(buffer, palstruct);
    SDL_SetSurfacePalette(overlay, palstruct);
    SDL_SetSurfacePalette(lps, palstruct);
    SDL_SetSurfacePalette(hps, palstruct);
    SDL_SetSurfacePalette(nt, palstruct);

    SDL_SetColorKey(buffer, SDL_TRUE, NES_TRANSPARENT);
    SDL_SetColorKey(overlay, SDL_TRUE, NES_TRANSPARENT);
    SDL_SetColorKey(lps, SDL_TRUE, NES_TRANSPARENT);
    SDL_SetColorKey(hps, SDL_TRUE, NES_TRANSPARENT);
    SDL_SetColorKey(nt, SDL_TRUE, NES_TRANSPARENT);

    tv=false; //start with table_view deactivated
    delay = false;

}

compositor::~compositor() {
    //SDL_DestroyRenderer(render);
    //SDL_DestroyTexture(texture);
    //SDL_DestroyWindow(screen);
    //SDL_FreePalette(palstruct);
    //SDL_FreeSurface(buffer);
    //SDL_FreeSurface(overlay);
    //SDL_FreeSurface(lps);
    //SDL_FreeSurface(hps);
    //SDL_FreeSurface(nt);
    //SDL_FreeSurface(buffer);
}


//These are set as hardware palette values. They're the full NES palette (disregarding the color emphasis bits).
//0->63 are NES values; 64->75 are values that I set in case I need them for my own purposes.
const int compositor::palette_table[228] = {0x80,0x80,0x80, 0x00,0x3D,0xA6, 0x00,0x12,0xB0, 0x44,0x00,0x96,
                                     0xA1,0x00,0x5E, 0xC7,0x00,0x28, 0xBA,0x06,0x00, 0x8C,0x17,0x00,
                                     0x5C,0x2F,0x00, 0x10,0x45,0x00, 0x05,0x4A,0x00, 0x00,0x47,0x2E,
                                     0x00,0x41,0x66, 0x00,0x00,0x00, 0x05,0x05,0x05, 0x05,0x05,0x05,
                                     0xC7,0xC7,0xC7, 0x00,0x77,0xFF, 0x21,0x55,0xFF, 0x82,0x37,0xFA,
                                     0xEB,0x2F,0xB5, 0xFF,0x29,0x50, 0xFF,0x22,0x00, 0xD6,0x32,0x00,
        /*NES Color Palette*/        0xC4,0x62,0x00, 0x35,0x80,0x00, 0x05,0x8F,0x00, 0x00,0x8A,0x55,
                                     0x00,0x99,0xCC, 0x21,0x21,0x21, 0x09,0x09,0x09, 0x09,0x09,0x09,
                                     0xFF,0xFF,0xFF, 0x0F,0xD7,0xFF, 0x69,0xA2,0xFF, 0xD4,0x80,0xFF,
                                     0xFF,0x45,0xF3, 0xFF,0x61,0x8B, 0xFF,0x88,0x33, 0xFF,0x9C,0x12,
                                     0xFA,0xBC,0x20, 0x9F,0xE3,0x0E, 0x2B,0xF0,0x35, 0x0C,0xF0,0xA4,
                                     0x05,0xFB,0xFF, 0x5E,0x5E,0x5E, 0x0D,0x0D,0x0D, 0x0D,0x0D,0x0D,
                                     0xFF,0xFF,0xFF, 0xA6,0xFC,0xFF, 0xB3,0xEC,0xFF, 0xDA,0xAB,0xEB,
                                     0xFF,0xA8,0xF9, 0xFF,0xAB,0xB3, 0xFF,0xD2,0xB0, 0xFF,0xEF,0xA6,
                                     0xFF,0xF7,0x9C, 0xD7,0xE8,0x95, 0xA6,0xED,0xAF, 0xA2,0xF2,0xDA,
                                     0x99,0xFF,0xFC, 0xDD,0xDD,0xDD, 0x11,0x11,0x11, 0x11,0x11,0x11,
                                     /*Black (64)     Grey 33% (65)   Grey 66% (66)   White (67)  */
  /*Utility colors (Non-NES)*/       0x00,0x00,0x00, 0x55,0x55,0x55, 0xAB,0xAB,0xAB, 0xFF,0xFF,0xFF,
                                     /*Yellow (68)     Magenta (69)    Cyan (70)       Red (71) */
                                     0xFF,0xFF,0x00, 0xFF,0x00,0xFF, 0x00,0xFF,0xFF, 0xFF,0x00,0x00,
                                     /*Green (72)      Blue (73)     Transparent (74)  Black (75) */
                                     0x00,0xFF,0x00, 0x00,0x00,0xFF, 0x00,0x00,0x00, 0x00,0x00,0x00};

void compositor::flip() {
    if(!render) return;

    SDL_Rect spr_render;
    spr_render.x = spr_render.y = 0; spr_render.w = 256; spr_render.h = 240;
    SDL_Rect spr_tv;
    spr_tv.x = 512; spr_tv.y = 0; spr_tv.w = 256; spr_tv.h = 240;

    SDL_Rect nt_render;
    nt_render.x = nt_render.y = 0; nt_render.w = 256; nt_render.h = 240;
    SDL_Rect nt_tv;
    nt_tv.x = nt_tv.y = 0; nt_tv.w = 512; nt_tv.h = 480;

    SDL_Rect all;
    all.x = all.y = 0; all.w = cur_x_res; all.h = cur_y_res;

    if(!buffer) {
        cout<<"Buffer is null; not flipping nuttin'!"<<endl;
        return;
    }
    else if(!(buffer->format->palette)) {
        cout<<"Palette is null; not flipping"<<endl;
    }

    //Draw low priority sprites, then all necessary sections of the background, then high priority sprites, then the overlay
    if(!tv) {

        SDL_BlitSurface(lps,NULL,buffer,&spr_render); //Draw low-priority sprites

        int ys = 0;
        while(nt_regions.size() != 0) { //Draw each section of the background that needs drawn
            SDL_Rect * v = nt_regions.front();
            nt_regions.pop_front();
            nt_render.y = ys;
            ys += v->h;
            SDL_BlitSurface(nt,v,buffer,&nt_render);
            delete v;
        }

        SDL_BlitSurface(hps,NULL,buffer,&spr_render); //Draw the high-priority sprites

    }
    else {
        SDL_BlitSurface(lps,NULL,buffer,&spr_tv); //Draw low-priority sprites
        SDL_BlitSurface(nt,NULL,buffer,&all); //Draw entire real-address name table
        { //Draw the extra name table crap
            SDL_Rect special;
            //Top-right mirror of table 1 + 3
            special.x = 512;
            special.y = 0;
            special.w = 256;
            special.h = 480;
            SDL_BlitSurface(nt,&special,buffer,&all);

            //Bottom-right mirror of table 1
            special.y = 480;
            special.h = 240;
            SDL_BlitSurface(nt,&special,buffer,&all);

            //Bottom-left mirror of table 1 +2
            special.x = 0;
            special.w = 512;
            SDL_BlitSurface(nt,&special,buffer,&all);
        }
        
        SDL_BlitSurface(hps,NULL,buffer,&spr_tv); //Draw the high-priority sprites
        //wait(20);
    }

    SDL_BlitSurface(overlay,NULL,buffer,&all); //Put the pset'ed overlay on top

    SDL_DestroyTexture(texture);

    texture = SDL_CreateTextureFromSurface(render, buffer);

    SDL_RenderClear(render);
    SDL_RenderCopy(render,texture,NULL,NULL);
    SDL_RenderPresent(render);

    //Clear all buffers
    fill(NES_TRANSPARENT, overlay);
    fill(NES_TRANSPARENT, lps);
    fill(NES_TRANSPARENT, nt);
    fill(NES_TRANSPARENT, hps);

    //if(!delay)
        //wait(10);
}

void compositor::fill(int color, SDL_Surface * buff/*=buffer*/) {
    if(!buffer) return;

    if(!buff) buff = buffer;
    SDL_Rect all;
    all.x=0; all.y=0; all.w=1024; all.h=1024;
    SDL_FillRect(buff,&all,color);
}

uint32_t compositor::get_color(int x,int y) {

    int wx=320;
    int wy=240;
    if(screen) {
        SDL_GetWindowSize(screen,&wx,&wy);
    }
    //cout<<"Requested ("<<x<<", "<<y<<"), current res: ("<<cur_x_res<<", "<<cur_y_res<<")"<<" window size: ("<<wx<<", "<<wy<<")"<<endl;
    if(x>=wx||y>=wy||x<0||y<0) {
        //cout<<"Requested ("<<x<<", "<<y<<"), current res: ("<<cur_x_res<<", "<<cur_y_res<<")"<<endl;
        return 0;
    }
    else {
        //cout<<"("<<x<<", "<<y;
        x = int(float(x) * (float(cur_x_res) / float(wx)));
        y = int(float(y) * (float(cur_y_res) / float(wy)));
        //cout<<") translated to ("<<x<<", "<<y<<")"<<endl;
    }

    size_t index = 0;
    if(buffer) {
        index = (y * buffer->pitch) + x;
    }

    int color_index = 1;
    if(screen) {
        color_index =  ((uint8_t *)(buffer->pixels))[index];
    }
    //                      blue                            green                                       red
    uint32_t color = palette_table[color_index *3+2] + ((palette_table[color_index * 3 + 1])<<(8)) + ((palette_table[color_index * 3])<<(16));
    return color;
}

void compositor::wait(int ms) {
    SDL_Delay(ms);
}

bool compositor::resize(int x,int y, bool maintain_aspect/*=true*/) {
    if(!screen) return true;
    if(!maintain_aspect) {
        SDL_FreeSurface(buffer);
        buffer = SDL_CreateRGBSurface(0,x,y,8,0,0,0,0);
        SDL_SetSurfacePalette(buffer, palstruct);
        SDL_SetColorKey(buffer, SDL_TRUE, NES_TRANSPARENT);

        SDL_FreeSurface(overlay);
        overlay = SDL_CreateRGBSurface(0,x,y,8,0,0,0,0);
        SDL_SetSurfacePalette(overlay, palstruct);
        SDL_SetColorKey(overlay, SDL_TRUE, NES_TRANSPARENT);

        cur_x_res=x;
        cur_y_res=y;

        SDL_SetWindowSize(screen, x, y);
        return true;
    }
    else {
        float aspect = float(base_x_res)/float(base_y_res);
        float aspect2 = float(x)/float(y);
        float aspect3 = aspect2 - aspect;
        if(aspect3 < 0) aspect3 *= -1.0;
        //if(aspect3 < 0.05 ) return false;
        int wx,wy;
        SDL_GetWindowSize(screen,&wx,&wy);
        int nx,ny;
        if(aspect < aspect2) {
            nx = int(float(y)*aspect);
            ny = y;
        }
        else {
            nx = x;
            ny = int(float(x)/aspect);
        }
        //cout<<"C-aspect: "<<aspect<<" new aspect: "<<aspect2<<" diff: "<<aspect3<<endl;
        //cout<<"Old size: ("<<wx<<", "<<wy<<"), new size: ("<<nx<<", "<<ny<<")"<<endl;
        if((nx != wx || ny != wy ) && aspect3 > 0.1) {
            SDL_SetWindowSize(screen, nx, ny);
        }
    }

    return true;
}

void compositor::pset(int x,int y,int color) {
    if(!overlay) return;
    //cout<<"X: "<<x<<" Y: "<<y<<" Color: "<<color<<endl;
    if(x>=overlay->w||y>=overlay->h||color>=76) {
        //cout<<"X: "<<x<<" Y: "<<y<<" Color: "<<color<<endl;
        //cout<<"cur_x_res: "<<cur_x_res<<" cur_y_res: "<<cur_y_res<<endl;
        //std::cout<<"Overlay: ("<<overlay->w<<", "<<overlay->h<<")"<<endl;
        std::cout<<"Pset: out of range ("<<x<<", "<<y<<"), "<<color<<endl;
        return;
    }
    //else cout<<"Pset: ("<<x<<", "<<y<<"), "<<color<<endl;
    //else {std::cout<<"pset ("<<x<<", "<<y<<"), "<<color<<std::endl;}
    ((uint8_t *)(overlay->pixels))[y*(overlay->pitch)+x]=color;
}

void compositor::table_view(bool active) {
    tv=active;
    if(tv) {
        resize(1024,512,false);
        base_x_res = 1024;
        base_y_res = 512;
    }
    else {
        resize(320,240,false);
        base_x_res = 320;
        base_y_res = 240;
    }
}

void compositor::speedup(bool up) {
    delay = up;
}

void compositor::blit_hps(SDL_Surface * s, int x, int y) {
    if(!hps) return;
    SDL_Rect pos;
    pos.x = x;
    pos.y = y;
    SDL_BlitSurface(s, NULL, hps, &pos);
}


void compositor::blit_lps(SDL_Surface * s, int x, int y) {
    if(!lps) return;
    SDL_Rect pos;
    pos.x = x;
    pos.y = y;
    SDL_BlitSurface(s, NULL, lps, &pos);
}

void compositor::blit_nt(SDL_Surface * s, int x, int y) {
    if(!nt) return;
    SDL_Rect pos;
    pos.x = x;
    pos.y = y;
    SDL_BlitSurface(s, NULL, nt, &pos);
}

void compositor::erase_nt(int x,int y) {
    if(!nt) return;
    SDL_Rect all;
    all.x=x; all.y=y; all.w=8; all.h=8;
    SDL_FillRect(nt,&all,NES_TRANSPARENT);
}

SDL_Palette * compositor::get_palette() {
    return palstruct;
}

void compositor::add_nt_area(int sx, int sy, int ex, int ey) {
    if(nt_regions.size() > 1000) return;
    SDL_Rect * temp = new SDL_Rect;
    temp->x = sx;
    temp->y = sy;
    temp->w = ex - sx;
    temp->h = ey - sy;
    nt_regions.push_back(temp);
}
