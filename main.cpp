#include "rom.h"
#include "mem.h"
#include "ppu3.h"
#include "cpu2.h"
#include "apu.h"
#include "util.h"
using namespace std;
#define ORIGIN 2

#ifdef _WIN32

//Windows doesn't provide getopt, so this is a simple implementation of it.
#include <ctime>
#include <string.h>
#include <stdio.h>

int opterr = 1,             /* if error message should be printed */
    optind = 1,             /* index into parent argv vector */
    optopt,                 /* character checked for validity */
    optreset;               /* reset getopt */
char *optarg;                /* argument associated with option */

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

/*
* getopt -- Parse argc/argv argument vector.
*/
int getopt(int nargc, char * const nargv[], const char *ostr) {
  static char *place = EMSG;              /* option letter processing */
  char *oli;                              /* option letter list index */

  if (optreset || !*place) {              /* update scanning pointer */
    optreset = 0;
    if (optind >= nargc || *(place = nargv[optind]) != '-') {
      place = EMSG;
      return (-1);
    }
    if (place[1] && *++place == '-') {      /* found "--" */
      ++optind;
      place = EMSG;
      return (-1);
    }
  }                                       /* option letter okay? */
  if ((optopt = (int)*place++) == (int)':' ||
    !(oli = (char *)(strchr(ostr, optopt)))) {
      /*
      * if the user didn't specify '-' as an option,
      * assume it means -1.
      */
      if (optopt == (int)'-')
        return (-1);
      if (!*place)
        ++optind;
      if (opterr && *ostr != ':')
        (void)printf("illegal option -- %c\n", optopt);
      return (BADCH);
  }
  if (*++oli != ':') {                    /* don't need argument */
    optarg = NULL;
    if (!*place)
      ++optind;
  }
  else {                                  /* need an argument */
    if (*place)                     /* no white space */
      optarg = place;
    else if (nargc <= ++optind) {   /* no arg */
      place = EMSG;
      if (*ostr == ':')
        return (BADARG);
      if (opterr)
        (void)printf("option requires an argument -- %c\n", optopt);
      return (BADCH);
    }
    else                            /* white space */
      optarg = nargv[optind];
    place = EMSG;
    ++optind;
  }
  return (optopt);                        /* dump back option letter */
}

#else
//Linux provides getopt, so include the header.
#include <getopt.h>
#endif
    
int run_nsf(rom& cart, apu& apui, mem& memi, cpu& cpui);

int main(int argc, char ** argv) {
    opterr = 0;
    char option;
    int res=1;
    int mapper=0;
    //cout<<"Argc: "<<argc<<endl;
    if(argc==1) {
        cout<<"Usage:\n\n./"<<argv[0]<<" [-d #] [-m #] <filename>\n\n\tOptions:\n\t-d Enables debug output. Options for # are:\n\t\t1\tCPU\n\t\t2\tMAIN\n\t\t3\tMEM\n\t\t4\tPPU\n\t\t5\tROM\n\t\t6\tUTIL\n\t\t7\tWAV\n\t\t7\tAPU\n";
        cout<<"\n\t-m manually specifies an iNES mapper number to use.\n";
    }

    while(argc>3&&(option=getopt(argc,argv,"d:x:m:"))!=-1) {
        switch(option) {
        case 'd':
            util::toggle_log(atoi(optarg));
            break;
        case 'x':
            res=atoi(optarg);
            break;
        case 'm':
            mapper=atoi(optarg);
            break;
        default:
            cerr<<"Unkown option '"<<optopt<<".\n";
            return 1;
        }
    }
    if(res==0)
        res=1; 
    std::string filename;
    if(optind<argc||argc==2) {
        filename=std::string(argv[optind]);
        cout<<"Loading "<<filename<<"..."<<endl;
    }
    else {
        cout<<"Give a single filename, like this: \n\n\t"<<argv[0]<<" rom.nes"<<endl;        
        return 1;
    }
        
//        if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0 ) {
    if( SDL_Init(SDL_INIT_EVERYTHING|SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER|SDL_INIT_NOPARACHUTE) < 0 ) {
        fprintf(stderr,"Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    //cout<<"Trying to open the joystick (if any)"<<endl;

    SDL_Joystick * js = NULL;
    int jscount = SDL_NumJoysticks();
    if(jscount > 0) {
        const char * name = SDL_JoystickNameForIndex(0);
        cout<<"Found joystick: "<<name<<endl;
        js = SDL_JoystickOpen(0);
    }

    //cout<<"Done with joystick stuff"<<endl;

    //cout<<"Going to load cartridge"<<endl;
    rom cart(filename, mapper);
    //cout<<"Cart loaded. Going to start ppu"<<endl;
    ppu ppui(cart,res);
    //cout<<"PPU started. Going to start apu."<<endl;
    apu apui;
    SDL_PauseAudioDevice(apui.get_id(), true);
    //cout<<"APU started. Going to bring up memory map"<<endl;
    mem memi(cart,ppui,apui);
    apui.setmem(&memi);
    //cout<<"Memory started. Going to start CPU"<<endl;
    cpu cpui(&memi,&apui,memi.get_rst_addr(), cart.isNSF());
    //cout<<"CPU started."<<endl;

    if(cart.isNSF()) {
        cout<<"Starting NSF player instead of main emulator"<<endl;
        return run_nsf(cart, apui, memi, cpui);
    }

    bool paused=false;
    //int time_stop;
    //long total_time;
    int pstart = SDL_GetTicks();
    //int delay = 0;
    //long delay_count = 0;
    //long delay_total = 0;
    SDL_PauseAudioDevice(apui.get_id(), false);
    while (1==1) {
        //cout<<"Start of event loop."<<endl;
        //int frame_start = SDL_GetTicks();
        SDL_Event event;
        //int curx,cury;
        int newx,newy;
        //bool changed;
        //cout<<"Polling and handling events"<<endl;
        while(SDL_PollEvent(&event)){  /* Loop until there are no events left on the queue */
            switch(event.type){  /* Process the appropiate event type */
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                {
                SDL_Scancode sc;
                int button = int(event.jbutton.button);
                int state = int(event.jbutton.state);
                //cout<<"Button: "<<state<<" state: "<<state<<endl;
                switch(button) {
                case 0: sc = SDL_SCANCODE_L; break;
                case 1: sc = SDL_SCANCODE_K; break;
                case 4: sc = SDL_SCANCODE_G; break;
                case 5: sc = SDL_SCANCODE_H; break;
                case 8: sc = SDL_SCANCODE_W; break;
                case 9: sc = SDL_SCANCODE_S; break;
                case 10: sc = SDL_SCANCODE_A; break;
                case 11: sc = SDL_SCANCODE_D; break;
                default: sc = SDL_SCANCODE_UNKNOWN;
                }
                if(state == 1)
                    memi.sendkeydown(sc);
                else
                    memi.sendkeyup(sc);
                }
                break;
            case SDL_JOYDEVICEADDED:
            case SDL_CONTROLLERDEVICEADDED:
                cout<<"Added joystick: "<<SDL_JoystickNameForIndex(event.jdevice.which)<<endl;
                if(js) {
                    SDL_JoystickClose(js);
                    js = NULL;
                }
                js = SDL_JoystickOpen(event.jdevice.which);
                break;
            case SDL_JOYDEVICEREMOVED:
            case SDL_CONTROLLERDEVICEREMOVED:
                cout<<"Removed joystick: "<<SDL_JoystickNameForIndex(event.jdevice.which)<<endl;
                if(js) {
                    SDL_JoystickClose(js);
                    js = NULL;
                }
                break;
            case SDL_KEYDOWN:  /* Handle a KEYDOWN event */         
                if(event.key.keysym.scancode==SDL_SCANCODE_Q||
                  (event.key.keysym.scancode==SDL_SCANCODE_C&&(event.key.keysym.mod==KMOD_RCTRL))||
                  (event.key.keysym.scancode==SDL_SCANCODE_C&&(event.key.keysym.mod==KMOD_LCTRL))) {
                    printf("You pressed q or ctrl-c. Exiting.\n");
                    cpui.print_details();
                    printf("%d frames rendered in %f seconds. (%f FPS)\n",ppui.get_frame(),float(clock())/float(CLOCKS_PER_SEC),float(ppui.get_frame())/(float(clock())/float(CLOCKS_PER_SEC)));
                    //SDL_PauseAudio(true);
                    cout<<"Calling SDL_Quit()"<<endl;
                    SDL_Quit();
                    cout<<"Called SDL_Quit()"<<endl;
                    return 0;
                }
                else if(event.key.keysym.scancode==SDL_SCANCODE_R) {
                    cpui.reset(memi.get_rst_addr());
                }
                else if(event.key.keysym.scancode==SDL_SCANCODE_P) {
                    paused=!paused;
                    if(paused) {
                        SDL_PauseAudioDevice(apui.get_id(), true);
                        printf("Paused emulation.\n");
                    }
                    else {
                        SDL_PauseAudioDevice(apui.get_id(), false);
                        printf("Unpaused emulation.\n");
                    }
                }
                else if(event.key.keysym.scancode >= SDL_SCANCODE_1 &&
                        event.key.keysym.scancode <= SDL_SCANCODE_0) {

                    std::cout<<"Saw scancode "<<event.key.keysym.scancode<<endl;
                    printf(" debug output #%d",int(event.key.keysym.scancode- (SDL_SCANCODE_1 - 1)));
                    //util::toggle_log(event.key.keysym.scancode-48);
                    printf("\n");
                }
                else if(event.key.keysym.scancode==SDL_SCANCODE_TAB) {
                    ppui.toggle_table_view();
                }
                else {
                    //printf("Keydown\n");
                    //printf("Time: %d\n",SDL_GetTicks());
                    memi.sendkeydown(event.key.keysym.scancode);
                }
                break;
            case SDL_KEYUP: /* Handle a KEYUP event*/
                //printf("Keyup\n");
                memi.sendkeyup(event.key.keysym.scancode);
                break;
            case SDL_MOUSEMOTION:
                memi.sendmousepos(event.motion.x, event.motion.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
                memi.sendmousedown(event.button.x,event.button.y);
                break;
            case SDL_MOUSEBUTTONUP:
                memi.sendmouseup(event.button.x,event.button.y);
                break;
            //case SDL_VIDEORESIZE:
            case SDL_WINDOWEVENT:
                switch(event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    newx=event.window.data1;
                    newy=event.window.data2;
                    if(newx > 0 && newy > 0) {
                        //changed=true;
                    }
                    //ppui.resize(newx,newy);
                    break;
                case SDL_WINDOWEVENT_EXPOSED:
                    //ppui.resize(1,1);
                    break;
                }
                break;
            case SDL_QUIT:
                //SDL_PauseAudio(true);
                SDL_Quit();
                cpui.print_details();
                printf("%d frames rendered in %f seconds. (%f FPS)\n",ppui.get_frame(),float(clock())/float(CLOCKS_PER_SEC),float(ppui.get_frame())/(float(clock())/float(CLOCKS_PER_SEC)));
                return 0;
                break;
            default: /* Report an unhandled event */
                //printf("I don't know what this event is! Flushing it.\n");
                SDL_FlushEvent(event.type);
                break;
            }
        }
        if(!paused) {
            //cout<<"Running the CPU"<<endl;
            int cpu_ret=cpui.run_ops();
            //cout<<"Ran the CPU"<<endl;
            if(cpu_ret < 0) {
                return 1;
            }
            else {
                //cout<<"Frame "<<ppui.get_frame()<<endl;

                int apu_id = apui.get_id();
                if(apu_id > 0) {
                    //cout<<"Locking audio...";
                    SDL_LockAudioDevice(apu_id);
                    //cout<<"done."<<endl;
                    apui.set_frame(ppui.get_frame());
                    apui.gen_audio();
                    //apui.dat.buffered += 735; Dude, do this in gen_audio, not here =/
                    //cout<<"Unlocking audio...";
                    SDL_UnlockAudioDevice(apu_id);
                    //cout<<"done."<<endl;
                }
                //cout<<SDL_GetTicks()<<": Frame: "<<ppui.get_frame()<<" Write pos: "<<apui.write_pos<<endl;
                //cout<<"Running the PPU"<<endl;
                int ppu_ret=ppui.calc();
                //cout<<"Ran the PPU"<<endl;
                if(ppu_ret>0) {
                    if(ppu_ret==1) {
                        //cout<<"Triggering NMI in CPU"<<endl;
                        cpui.trigger_nmi();
                    }
                }
            }
        }

        //Calculates the time that the frame should end at, and delays until that time
        //cout<<"Timing two"<<endl;
        int frames = ppui.get_frame();
        int now = SDL_GetTicks();
        int delay = pstart + int(double(frames) * double(1000) / double(60)) - now;
        //cout<<"Frame: "<<frames<<" now: "<<now<<" delay: "<<delay<<endl;       
        if(delay > 0) {
            SDL_Delay(delay);
        }
    }
    return 0;
}

unsigned int get_choice(rom& cart) {
    unsigned int choice = 0;
    while(choice < 1 || choice > cart.get_song_count()) {
        cout<<"Choose a song number between 1 and "<<dec<<cart.get_song_count()<<": ";
        cout.flush();
        cin>>choice;
    }
    return choice;
}

bool init_nsf(rom& cart, mem& memi, cpu& cpui, unsigned int choice = 0) {
    cout<<"Playing song "<<dec<<choice<<" out of "<<cart.get_song_count()<<endl;
    cpui.reset(cart.get_rst_addr());
    cpui.set_acc(choice-1);
    cpui.set_x(0);
    for(int i=0;i<0x800;++i)
        memi.write(i,0);
    for(int i=0x6000;i<0x8000;++i)
        memi.write(i,0);
    for(int i=0x4000;i<0x4014;++i)
        memi.write(i,0);
    memi.write(0x4015, 0x0f);
    memi.write(0x4017, 0x40);
    cart.reset_map();
    int ret = 1;
    while(ret) {
        ret = cpui.run_next_op();
        if(ret < 0) return false;
    }
    cpui.reset(cart.get_nmi_addr());
    cout<<"I think I init'ed the program properly."<<endl;
    return true;
}

int run_nsf(rom& cart, apu& apui, mem& memi, cpu& cpui) {
    bool paused=false;
    int frame = 0;
    int choice = get_choice(cart);
    bool success = init_nsf(cart, memi, cpui,choice);
    if(!success) {
        cout<<"Failed to init nsf."<<endl;
        return 1;
    }
    SDL_PauseAudioDevice(apui.get_id(), false);
    int pstart = SDL_GetTicks();
    while (1==1) {
exit_poll_loop:
        SDL_Event event;
        while(SDL_PollEvent(&event)){  /* Loop until there are no events left on the queue */
            switch(event.type){  /* Process the appropiate event type */
            case SDL_KEYDOWN:  /* Handle a KEYDOWN event */         
                if(event.key.keysym.scancode==SDL_SCANCODE_Q||
                  (event.key.keysym.scancode==SDL_SCANCODE_C&&(event.key.keysym.mod==KMOD_RCTRL))||
                  (event.key.keysym.scancode==SDL_SCANCODE_C&&(event.key.keysym.mod==KMOD_LCTRL))) {
                    printf("You pressed q or ctrl-c. Exiting.\n");
                    cpui.print_details();
                    //SDL_PauseAudio(true);
                    cout<<"Calling SDL_Quit()"<<endl;
                    SDL_Quit();
                    cout<<"Called SDL_Quit()"<<endl;
                    return 0;
                }
                else if(event.key.keysym.scancode==SDL_SCANCODE_A) {
                    choice--;
                    if(choice<1) choice = cart.get_song_count();
                    SDL_PauseAudioDevice(apui.get_id(), true);
                    init_nsf(cart, memi, cpui, choice);
                    SDL_PauseAudioDevice(apui.get_id(), false);
                    goto exit_poll_loop;
                }
                else if(event.key.keysym.scancode==SDL_SCANCODE_D) {
                    choice++;
                    if(choice > (int)cart.get_song_count()) choice = 1;
                    SDL_PauseAudioDevice(apui.get_id(), true);
                    init_nsf(cart, memi, cpui, choice);
                    SDL_PauseAudioDevice(apui.get_id(), false);
                    goto exit_poll_loop;
                }
                else if(event.key.keysym.scancode==SDL_SCANCODE_R) {
                    SDL_PauseAudioDevice(apui.get_id(), true);
                    init_nsf(cart,memi,cpui);
                    SDL_PauseAudioDevice(apui.get_id(), false);
                    goto exit_poll_loop;
                }
                else if(event.key.keysym.scancode==SDL_SCANCODE_P) {
                    paused=!paused;
                    if(paused) {
                        printf("Paused emulation.\n");
                        SDL_PauseAudioDevice(apui.get_id(), true);
                    }
                    else {
                        printf("Unpaused emulation.\n");
                        SDL_PauseAudioDevice(apui.get_id(), false);
                    }
                }
                else {
                    //printf("Keydown\n");
                    //printf("Time: %d\n",SDL_GetTicks());
                }
                break;
            case SDL_KEYUP: /* Handle a KEYUP event*/
                //printf("Keyup\n");
                break;
            case SDL_QUIT:
                //SDL_PauseAudio(true);
                SDL_Quit();
                cpui.print_details();
                return 0;
                break;
            default: /* Report an unhandled event */
                //printf("I don't know what this event is! Flushing it.\n");
                SDL_FlushEvent(event.type);
                break;
            }
        }
        if(!paused) {
            //cout<<"Running the CPU"<<endl;
            int cpu_ret = 1;
            int cycle_count = 0;
            bool reset = true;
            while(cpu_ret && cpu_ret > 0) {
                cpu_ret=cpui.run_next_op();
                cycle_count += (3*cpu_ret);
                //if(cycle_count > CLK_PER_FRAME) { 
                //    reset = false;
                //    break;
                //}
            }

            if(reset) {
                cpui.reset(cart.get_nmi_addr());
            }

            //cout<<"Ran the CPU"<<endl;
            if(cpu_ret < 0) {
                return 1;
            }
            else {
                //cout<<"Frame "<<ppui.get_frame()<<endl;

                int apu_id = apui.get_id();
                if(apu_id > 0) {
                    //cout<<"Locking audio...";
                    SDL_LockAudioDevice(apu_id);
                    //cout<<"done."<<endl;
                    apui.set_frame(frame);
                    apui.gen_audio();
                    //apui.dat.buffered += 735; Dude, do this in gen_audio, not here =/
                    //cout<<"Unlocking audio...";
                    SDL_UnlockAudioDevice(apu_id);
                    //cout<<"done."<<endl;
                }
            }
        }

        //Calculates the time that the frame should end at, and delays until that time
        //cout<<"Timing two"<<endl;
        int now = SDL_GetTicks();
        frame++;
        cpui.increment_frame();
        int delay = pstart + int(double(frame) * double(1000) / double(60)) - now;
        //cout<<"Frame: "<<frames<<" now: "<<now<<" delay: "<<delay<<endl;       
        if(delay > 0) {
            SDL_Delay(delay);
        }
    }
}
