#include "rom.h"
#include<stdlib.h>
#include "nes_constants.h"
#include "mapper.h"
#include "mapper_001_mmc1.h"
#include "mapper_002_unrom.h"
#include "mapper_003_cnrom.h"
#include "mapper_011_color_dreams.h"
#include "mapper_nsf.h"
#include<assert.h>
#define ORIGIN 5

rom::rom(std::string filename, int m) : file(filename), mapper_num(m) {
    valid=load(filename);
    if(valid) {
        print_info();
    }
    else {
        printf("Trying to load as NSF instead\n");
        valid=load_nsf(filename);
        if(nsf) {
            print_nsf_info();
        }
        else {
            print_info();
        }
        if(!valid) {
            printf("Problem loading %s. Make sure it exists and you can read it.\n",filename.c_str());
            exit(1);
        }
    }
    //printf("CHR_PAGE_SIZE: %d sizeof crom: %d crom pages: %d",CHR_PAGE_SIZE,crom_pages,CHR_PAGE_SIZE*crom_pages);
}

bool rom::load(std::string& filename) {
    std::ifstream filein;
    filein.open(filename.c_str(),std::ios::binary|std::ios::in);
    if(!(filein&&filein.is_open())) {
        filein.close();
        return false;
    }
    //get filesize
    filein.seekg(0,std::ios::end);
    filesize=int(filein.tellg());
    filein.seekg(0,std::ios::beg);
    if(filesize<MIN_FILE_SIZE) {
        filein.close();
        return false;
    }
    //parse the header, assign appropriate values to properties
    header.resize(16,0);
    filein.read(reinterpret_cast<char*>(&header[0]),16);

    std::cout<<"Header: ";
    for(int i=0;i<16;++i) {
        printf(" %02x ",header[i]);
    }
    std::cout<<"\n        ";
    for(int i=0;i<16;++i) {
        printf("%03d ", header[i]);
    }
    std::cout<<"\n        ";;
    for(int i=0;i<16;++i) {
        if(header[i] >= 32 && header[i] < 127)
            printf("  %c ", header[i]);
        else printf("    ");
    }
    std::cout<<std::endl;

    if(header[0]!='N'||header[1]!='E'||header[2]!='S'||header[3]!=0x1a) {
        printf("(%02x)  (%02x)  (%02x)  (%02x)", header[0],header[1],header[2],header[3]);
        filein.close();
        return false;
    }

    prom_pages=(int)header[4];
    crom_pages=(int)header[5];

    if(0x01&header[6]) {
        mirror_mode=VERT; //vertical mirrored
    }
    else if(0x08&header[6]) {
        mirror_mode=FOUR_SCR; //4 screen mirror
    }
    else {
        mirror_mode=HORIZ; //horizontal mirror
    }

    if(mapper_num == 0) {
        //std::cout<<std::hex<<(header[6]>>(4))<<" "<<(header[7]&0xF0)<<std::endl;
        mapper_num=((header[6]>>(4)));//|(header[7]&0xF0));//often incorrect
        if(header[7] != 'D') mapper_num |= (header[7]&0xF0);
    }

    sram=((0x02&header[6])>0)?true:false;
    trainer=((0x04&header[6])>0)?true:false;

    prom.resize(prom_pages*PRG_PAGE_SIZE);
    prom_w.resize(prom_pages * PRG_PAGE_SIZE);
    filein.read(reinterpret_cast<char*>(&prom[0]),PRG_PAGE_SIZE*prom_pages);
    for(int i=0;i<prom_pages*PRG_PAGE_SIZE-1;++i)
        prom_w[i] = prom[i] | (prom[i+1]<<(8));

    if(crom_pages == 0) {//For ROMs with no CHR-ROM, create one page of RAM for the PRG to fill
        crom.resize(CHR_PAGE_SIZE,0);
    }
    else {//Read in the appropriate number of CHROM pages, if any exist.
          //cpage can be used for bank switching if necessary.
        crom.resize(crom_pages * CHR_PAGE_SIZE, 0);
        filein.read(reinterpret_cast<char*>(&crom[0]),CHR_PAGE_SIZE*crom_pages);
    }

    switch(mapper_num) {
    case 0:
        map = new mapper(this);
        break;
    case 1:
        map = new mapper_001(this);
        break;
    case 2:
        map = new mapper_002(this);
        break;
    case 3:
        map = new mapper_003(this);
        break;
    case 11:
        map = new mapper_011(this);
        break;
    default:
        std::cout<<"Mapper not implemented yet."<<std::endl;
        filein.close();
        return false;
    }

    filein.close();

    //Find the reset.power-on bit vector
    int startl=get_pbyte(RST_INT_INSERTION);
    int starth=get_pbyte(RST_INT_INSERTION+1);
    rst_addr=startl|starth<<8;
    //if(get_pword(RST_INT_INSERTION) != rst_addr) std::cout<<"Unexpected error!"<<std::endl;

    //Find the IRQ bit vector
    startl=get_pbyte(IRQ_INT_INSERTION);
    starth=get_pbyte(IRQ_INT_INSERTION+1);
    irq_addr=startl|starth<<8;
    //if(get_pword(IRQ_INT_INSERTION) != irq_addr) std::cout<<"Unexpected error!"<<std::endl;

    //Find the NMI bit vector
    startl=get_pbyte(NM_INT_INSERTION);
    starth=get_pbyte(NM_INT_INSERTION+1);
    nmi_addr=startl|starth<<8;
    //if(get_pword(NM_INT_INSERTION) != nmi_addr) std::cout<<"Unexpected error!"<<std::endl;

    return true;
}

bool rom::load_nsf(std::string& filename) {
    std::ifstream filein;
    filein.open(filename.c_str(),std::ios::binary|std::ios::in);
    if(!(filein&&filein.is_open())) {
        filein.close();
        return false;
    }
    //get filesize
    filein.seekg(0,std::ios::end);
    filesize=int(filein.tellg());
    filein.seekg(0,std::ios::beg);
    if(filesize<0x100) {
        filein.close();
        return false;
    }
    //parse the header, assign appropriate values to properties
    header.resize(0x80,0);
    filein.read(reinterpret_cast<char*>(&header[0]),0x80);

    std::cout<<"Header: ";
    for(int i=0;i<0x20;++i) {
        printf(" %02x ",header[i]);
    }
    std::cout<<"\n        ";
    for(int i=0;i<0x20;++i) {
        printf("%03d ", header[i]);
    }
    std::cout<<"\n        ";;
    for(int i=0;i<0x20;++i) {
        if(header[i] >= 32 && header[i] < 127)
            printf("  %c ", header[i]);
        else printf("    ");
    }
    std::cout<<std::endl;

    if(header[0]!='N'||header[1]!='E'||header[2]!='S'||header[3]!='M'||header[4]!=0x1a) {
        printf("(%02x)  (%02x)  (%02x)  (%02x) (%02x)", header[0],header[1],header[2],header[3],header[4]);
        printf("Header doesn't match expected NSF header.\n");
        filein.close();
        return false;
    }
    
    if(header[5] != 1) {
        printf("I don't handle files of version %d.\n", header[5]);
        filein.close();
        return false;
    }
    song_count = header[6];

    song_index = header[7] - 1;

    //Find the load addr
    int startl=header[0x08];
    int starth=header[0x09];
    load_addr=startl|starth<<8;

    //Find the song-init/power-on vector
    startl=header[0x0a];
    starth=header[0x0b];
    rst_addr=startl|starth<<8;

    //Find the NMI bit vector/song iteration vector
    startl=header[0x0c];
    starth=header[0x0d];
    nmi_addr=startl|starth<<8;
    //if(get_pword(NM_INT_INSERTION) != nmi_addr) std::cout<<"Unexpected error!"<<std::endl;

    //Ticks for PAL+NTSC in 1uS increments (play speed)
    startl=header[0x6e];
    starth=header[0x6f];
    ntsc_ticks = startl|starth<<8;

    startl=header[0x78];
    starth=header[0x79];
    pal_ticks = startl|starth<<8;

    if(header[0x7a] & 0x2) {
        pal = ntsc = true;
    }
    else {
        pal = (header[0x7a] & 0x1);
        ntsc = !pal;
    }

    if(!ntsc) {
        printf("NTSC not available, and that's all I'm supporting right now.\n");
        return false;
    }

    uses_banks = false;
    map = new mapper_nsf(this);
    for(int i=0;i<8;++i) {
        if(header[0x70+i]) uses_banks = true;
    }

    if(uses_banks) {
        std::cout<<std::hex<<load_addr<<"\t"<<filesize-0x80<<" "<<filesize/0x1000<<filesize % 0x1000<<std::endl;
        load_addr &= 0xfff;
        size_t prom_size = filesize - 0x80 + (load_addr&0xFFF);
        //assert(prom_size > 0 && prom_size + 0x8000 > rst_addr);
        if(prom_size < 0x8000) prom_size = 0x8000;
        prom.resize(prom_size, 0);
        for(int i=0;i<8;++i) {
            map->put_pbyte(0, header[0x70+i], 0x5ff8+i);
        }
        load_addr += 0x8000;
    }
    else {
        size_t prom_size = 0x8000;
        //size_t prom_size = filesize - 0x80 + load_addr;
        //assert(prom_size > 0 && prom_size + 0x8000 > rst_addr);
        prom.resize(prom_size, 0);
        //prom.resize(0x8000,0);
    }
    std::cout<<"PROM size: "<<std::dec<<prom.size()<<"(0x"<<std::hex<<prom.size()<<")"<<std::endl;
    prom_w.resize(prom.size());

    filein.read(reinterpret_cast<char*>(&prom[load_addr-0x8000]),filesize-0x80);
    for(unsigned int i=0;i<prom.size()-1;++i) {
        prom_w[i] = prom[i] | (prom[i+1]<<(8));
    }

    mirror_mode=VERT; //vertical mirrored
    crom.resize(CHR_PAGE_SIZE,0);

    filein.close();

    nsf = true;
    sram = true;

    return true;
}

bool rom::isValid() {
    return valid;
}

bool rom::isNSF() {
    return nsf;
}

bool rom::has_sram() {
    return sram;
}

const std::string& rom::filename() {
    return file;
}

const unsigned int rom::get_song_count() {
    if(nsf) return song_count;
    return 0;
}

const unsigned int rom::get_default_song() {
    if(nsf) return song_index;
    return 0;
}

const unsigned int rom::get_header(const unsigned int addr) {
    if(addr >= header.size()) return 0;
    else return header[addr];
}

void rom::print_info() {
    std::cout<<"Filename: "<<file.c_str()<<std::endl;
    std::cout<<"Filesize: "<<filesize<<std::endl;
    std::cout<<"Valid: "<<(valid?"true":"false")<<std::endl;
    std::cout<<"PROM pages: "<<prom_pages<<std::endl;
    std::cout<<"CROM pages: "<<crom_pages<<std::endl;
    std::cout<<"Trainer: "<<(trainer?"true":"false")<<std::endl;
    std::cout<<"Mirror mode: ";
    switch(mirror_mode) {
        case VERT:
            std::cout<<"Vertical mirroring"<<std::endl;
            break;
        case HORIZ:
            std::cout<<"Horizontal mirroring"<<std::endl;
            break;
        case FOUR_SCR:
            std::cout<<"4-Screen mirroring"<<std::endl;
            break;
    }
    std::cout<<"Contains SRAM: "<<(sram?"true":"false")<<std::endl;
    std::cout<<"Mapper: "<<mapper_num<<std::endl;

    printf("RST Insertion point: %04x\n",rst_addr);
    printf("IRQ Insertion point: %04x\n",irq_addr);
    printf("NMI Insertion point: %04x\n",nmi_addr);
}

void rom::print_nsf_info() {
    std::cout<<"Filename: "<<file.c_str()<<std::endl;
    std::cout<<"Filesize: "<<filesize<<std::endl;
    std::cout<<"Valid: "<<(valid?"true":"false")<<std::endl;
    printf("Init Insertion point: %04x\n",rst_addr);
    printf("Load Insertion point: %04x\n",load_addr);
    printf("Tick Insertion point: %04x\n",nmi_addr);
    printf("Extra hardware flag: %02x", header[0x7b]);
    if(header[0x07b]) {
        printf(" (Extra sound chip support specified, and I don't currently support that.)");
    }
    printf("\n");

    printf("Songs in file: %d\n", song_count);
    printf("Starting song: %d\n", song_index+1);
    printf("Load addr: %04x\nInit addr: %04x\nPlay addr: %04x\n", load_addr, rst_addr, nmi_addr);
    printf("Song name: %s\nArtist: %s\nCopyright: %s\n", &header[0x0e], &header[0x2e], &header[0x4e]);

    printf("NTSC ticks (uS): %d\nPAL ticks (uS): %d\n", ntsc_ticks, pal_ticks);

    printf("NTSC: %d\n PAL: %d\n", ntsc,pal);

    printf("Bank map\n--------\n");
    for(int i=0;i<8;++i) {
        printf("%02x ", header[0x70+i]);
    }
    printf("\n");
}

rom::~rom() {
}

const unsigned int rom::get_rst_addr() {
    return rst_addr;
}
const unsigned int rom::get_nmi_addr() {
    return nmi_addr;
}
const unsigned int rom::get_irq_addr() {
    return irq_addr;
}

const unsigned int rom::get_mode() {
    return mirror_mode;
}

const unsigned int rom::get_pbyte(const unsigned int addr) {
    return map->get_pbyte(addr);
}

const unsigned int rom::get_pword(const unsigned int addr) {
    return map->get_pword(addr);
}

const unsigned int rom::get_page(const unsigned int addr) {
    return map->get_page(addr);
}

//returns raw character byte. Mainly used for register-based PPU access, and regenerating the
//rendering tables stored in the ppu class.
const unsigned int rom::get_cbyte(const unsigned int addr) {
    return map->get_cbyte(addr);
    //std::cout<<std::hex<<"CHR_HI_OFFSET: 0x"<<chr_hi_offset<<" (reading addr=0x"<<addr<<")"<<std::dec<<std::endl;
    //return crom[addr%(CHR_PAGE_SIZE*crom_pages)];
    if(addr>=8192) {
        std::cout<<"Trying to get CHR byte at address: "<<addr<<std::endl;
        return 0;
    }
    if(addr < 0x1000) return crom[addr+chr_lo_offset];
    else return crom[addr-0x1000+chr_hi_offset];
}

void rom::put_pbyte(const unsigned int cycle, const unsigned int val,const unsigned int addr) {
    map->put_pbyte(cycle, val, addr);
}

bool rom::put_cbyte(const unsigned int val,const unsigned int addr) {
    return map->put_cbyte(val, addr);
}

int rom::changed_crom() {
    return map->changed_crom();
}
void rom::ppu_change(unsigned int cycle, unsigned int addr, unsigned int val) {
    map->ppu_change(cycle, addr, val);
}

rom::ppu_change_t rom::cycle_forward(unsigned int cycle) {
    return map->cycle_forward(cycle);
}

void rom::reset_map() {
    map->reset_map();
}

