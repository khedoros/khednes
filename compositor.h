#pragma once
#include "SDL2/SDL.h"
#include <assert.h>
#include <iostream>
#include <list>
#include "util.h"

using namespace std;

class compositor {
private:
    SDL_Window * screen;
    SDL_Renderer * render;

    SDL_Surface * buffer; //Output buffer
    SDL_Texture * texture; //Texture used for output

    SDL_Surface * overlay; //overlay surface
    SDL_Surface * lps; //Low-priority sprite compositing buffer
    SDL_Surface * nt; //Name table compositing buffer
    SDL_Surface * hps; //High-priority sprite compositing buffer

    SDL_Color tv_palette[76];
    SDL_Palette *palstruct;
    static const int palette_table[228];
    int cur_x_res, cur_y_res, base_x_res, base_y_res;
    bool tv;
    bool delay;
    list<SDL_Rect *> nt_regions;
    
public:
    compositor(int res);
    ~compositor();
    void flip();
    void fill(int color, SDL_Surface * buff = NULL);
    void wait(int ms);
    void pset(int x,int y,int color);
    bool resize(int x,int y, bool maintain_aspect = true);
    void table_view(bool active);
    void speedup(bool up);
    void blit_lps(SDL_Surface * s, int x, int y);
    void blit_hps(SDL_Surface * s, int x, int y);
    void blit_nt(SDL_Surface * s, int x, int y);
    SDL_Palette * get_palette();
    void erase_nt(int x,int y);
    void add_nt_area(int sx, int sy, int ex, int ey);
    uint32_t get_color(int x, int y);
};

