#include<stdio.h>
#include<stdint.h>
#include "util.h"
#define ORIGIN 6

bool util::init=false;
std::bitset<1000> util::active;

void util::toggle_log(int origin) {
    if(!init) {
        for(int i=0;i<1000;++i) active[i] = false;
        init=true;
    }
    active[origin]=!(active[origin]);
    switch(origin) {
    case 1:
        printf("1 (CPU)");
        break;
    case 2:
        printf("2 (MAIN)");
        break;
    case 3:
        printf("3 (MEM)");
        break;
    case 4:
        printf("4 (PPU)");
        break;
    case 5:
        printf("5 (ROM)");
        break;
    case 6:
        printf("6 (UTIL)");
        break;
    case 7:
        printf("7 (WAV)");
        break;
    case 8:
        printf("8 (APU)");
        break;
    }
}

bool util::is_active(int i) {
    return active[i];
}

int util::debug(int orig,const char * fmt,...) {
    if(!init) {
        for(int i=0;i<1000;++i) {
            active[i]=false;
        }
        init=true;
    }

    if(active[orig]||(orig==999&&active[1])) {
        switch(orig) {
                case 1:
                        printf("CPU: ");
                        break;
                case 2:
                        printf("MAIN: ");
                        break;
                case 3:
                        printf("MEM: ");
                        break;
                case 4:
                        printf("PPU: ");
                        break;
                case 5:
                        printf("ROM: ");
                        break;
                case 6:
                        printf("UTIL: ");
                        break;
                case 7:
                        printf("WAV: ");
                        break;
                case 8:
                        printf("APU: ");
                        break;
                default:
                        break; //Don't print origin label
        }
        va_list ap;
        va_start(ap,fmt);
        return vprintf(fmt,ap);
    }
    else {
        return 0;
    }
}

int util::debug(const char * fmt,...) {
    va_list ap;
    va_start(ap,fmt);
    return vprintf(fmt,ap);
}
