#include<unistd.h>
#include<iostream>
#include<list>
#include<vector>
#include "rom.h"
using namespace std;

unsigned int sizes[] = {};
unsigned int types[] = {};

int main() {
    rom game("duckhunt.nes", 0);
    if(game.isValid()) {
        cout<<"Loaded the game."<<endl;
        game.print_info();
    }
    else {
        return 1;
    }

    list<unsigned int> addrs;
    vector<bool> visited(0x8000);
    addrs.push_back(game.get_rst_addr());
    addrs.push_back(game.get_nmi_addr());
    addrs.push_back(game.get_irq_addr());

    while(addrs.size() > 0) {

    }

    return 0;
}
