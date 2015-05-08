#include "apu.h"
#include "assert.h"
#include<iostream>
using namespace std;

uint8_t apu::noise_states[2][32767];
uint16_t apu::noise_freq_table[16];
uint8_t apu::pal_len_table[8];
uint8_t apu::ntsc_len_table[8];
uint8_t apu::general_len_table[16];
uint8_t apu::TRI_STATES[TRI_LEN];
uint8_t apu::SQ_STATES[4][SQ_LEN];
uint16_t apu::NOISE_LEN[2];

void my_callback(void * ud, Uint8 * stream, int len) {
    //cout<<"Actual callback size: "<<len<<endl;
    userdata * dat = (userdata *)(ud);
    //dat->first = !(dat->first);
    //Copy the audio data
    //cout<<"Read: Index: "<<(dat->apui->buffer_read_pos)<<" Samples: "<<(len*(dat->apui->buffer_read_pos))<<" -> "<<(len*(dat->apui->buffer_read_pos+1))<<" len: "<<len<<endl;
    memcpy(stream, &dat->buffer[len*(dat->apui->buffer_read_pos)], len);
    if(dat->buffered >= len) {
        dat->buffered -= len;
        dat->apui->buffer_read_pos++;
        dat->apui->buffer_read_pos%=dat->apui->SAMPLES_PER_FRAME;
        //cout<<"Consume "<<len<<endl;
    }
    else {
        cout<<"Underbuffered by "<<len - dat->buffered<<" bytes!"<<endl;
        //memset(stream + dat->buffered, 0, len - dat->buffered);
    }
}

SDL_AudioDeviceID apu::get_id() {
    return id;
}

void apu::set_frame(int f) {
    frame = f;
}

int apu::get_frame() {
    return frame;
}
void apu::init() {

    //dat.buffer = &(buffer[0]); Set this after the number of samples has been determined
    dat.apui = this;
    dat.first = 1;
    dat.buffered = 0;

    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_U8;
    desired.channels = 1;

    //#ifndef _WIN32
    //desired.samples = SAMPLES_PER_FRAME * 2; //Works for PulseAudio
    //#else
    desired.samples = 1024;                  //Works for DirectSound
    //#endif
    desired.callback = my_callback;
    desired.userdata = &dat;

    int init_success = 0;

    #ifdef _WIN32
    const char * desired_driver = "directsound";
    #else
    const char * desired_driver = "pulseaudio";
    #endif
    for (int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
        const char* driver_name = SDL_GetAudioDriver(i);
        printf("Audio driver #%d: %s\n", i, driver_name);
        if(strcmp(driver_name, desired_driver) == 0) {
            cout<<"Going to init "<<desired_driver<<" driver."<<endl;
            init_success = SDL_AudioInit(desired_driver);
        }
    }
    
    const char * device_name = "";
    if(init_success == 0) {
        cout<<"I think I successfully init'd the driver. Here are the "<<SDL_GetNumAudioDevices(0)<<" audio devices it provides: "<<endl;
        for(int i=0;i<SDL_GetNumAudioDevices(0);++i) {
            device_name = SDL_GetAudioDeviceName(i,0);
            #ifdef _WIN32
            if(i == 0 && device_name) id = SDL_OpenAudioDevice(device_name, 0, &desired, &obtained, 0);
            #endif
            
            if(device_name) {
                printf("Audio device #%d: %s\n", i, device_name);
            }
            else
                printf("Audio device #%d: NULL!?!\n", i);
        }
    }

    #ifndef _WIN32
    id = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    #endif    

    if(id <= 0) {
        cerr<<"Couldn't open the audio device: "<<SDL_GetError()<<endl;
        //return 1;
    }
    else {
        if(SDL_GetCurrentAudioDriver()) {
            cout<<"Opened audio device with driver: "<<SDL_GetCurrentAudioDriver()<<endl;
        }
        else {
            cout<<"Doesn't seem like a driver was successfully init'd!"<<endl;

        }
    }

    if(id > 0) {
    //if(desired.freq != obtained.freq || desired.format != obtained.format || desired.channels != obtained.channels || desired.samples != obtained.samples) {
        //cout<<"Desired:\n\tFreq: "<<desired.freq<<"\n\tFormat: "<<desired.format<<"\n\tChannels: "<<int(desired.channels)<<"\n\tSamples: "<<desired.samples<<endl<<endl;
        cout<<"Obtained:\n\tFreq: "<<obtained.freq<<"\n\tFormat: "<<obtained.format<<"\n\tChannels: "<<int(obtained.channels)<<"\n\tSamples: "<<obtained.samples<<endl;
            //cout<<"Bufsize: "<<bufsize<<endl;
        size_t bufsize = SAMPLES_PER_FRAME * obtained.samples;
        buffer.resize(bufsize,0);
        dat.buffer = &(buffer[0]);
    //}
    //cout<<"byte size: "<<obtained.size<<endl;

    //if(obtained.freq != SAMPLE_RATE) {
    //    cout<<"Didn't get the frequency I wanted. Should be interesting."<<endl;
    //}
    }
}

void apu::generate_arrays() {
    shift_reg val;
    val.val = 1;
    bool start = true;
    uint16_t count = 0;
    //generate the long noise values
    while (start || val.val != 1) {
        start = false;
        unsigned char bop = val.bit13 ^ val.bit14;
        noise_states[0][count] = ((val.bit13 ^ val.bit14)?0:1);
        val.val <<=(1);
        val.val |= (unsigned int)(bop);
        ++count;
    }
    //cout<<"Long count: "<<count<<endl;
    assert(count == 32767);
    val.val = 1;
    start = true;
    count = 0;
    //generate the short noise values
    while (start || val.val != 1) {
        start = false;
        uint8_t bop = val.bit8 ^ val.bit14;
        noise_states[1][count] = ((val.bit8 ^ val.bit14)?0:1);
        val.val <<=(1);
        val.val |= bop;
        ++count;
    }
    //cout<<"Short count: "<<count<<endl;
    assert(count == 93);

    uint16_t n_f_t[16] = {0x002, 0x004, 0x008, 0x010,
                          0x020, 0x030, 0x040, 0x050,
                          0x065, 0x07F, 0x0BE, 0x0FE,
                          0x17D, 0x1FC, 0x3F9, 0x7F2};

    uint8_t g_l_t[16] = { 0x7f, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                          0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

    for(uint8_t i=0;i<16;++i) {
        noise_freq_table[i] = n_f_t[i];
        general_len_table[i] = g_l_t[i];
    }

    uint8_t p_l_t[8] = { 0x05, 0x0A, 0x14, 0x28, 0x50, 0x1E, 0x07, 0x0D };
    uint8_t n_l_t[8] = { 0x06, 0x0C, 0x18, 0x30, 0x60, 0x24, 0x08, 0x10 };

    for(uint8_t i=0;i<8;++i) {
        pal_len_table[i] = p_l_t[i];
        ntsc_len_table[i] = n_l_t[i];
    }

    uint8_t t_s[TRI_LEN] = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    for(uint8_t i=0;i<32;++i) TRI_STATES[i] = t_s[i];

    uint8_t s_s[4][SQ_LEN] = {{0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
                              {0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
                              {0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0},
                              {1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1}};

    for(uint8_t i=0;i<4;++i)
        for(uint8_t j=0;j<16;++j)
            SQ_STATES[i][j] = s_s[i][j];

    uint16_t n_l[2] = {32767, 93};
    NOISE_LEN[0] = n_l[0];
    NOISE_LEN[1] = n_l[1];
}


apu::apu() {
    //std::cout<<"apu constructor"<<std::endl;
    stat.val = 0;
    linear_cntr = 0;
    enable_linear_cntr = false;
    pin1_enabled = 0;
    pin2_enabled = 0;
    noise_flip_flop = false;
    framecounter_pos = 0;
    framecounter_reset = 89490;
    enable_irq = false;
    counter_mode = 0;
    pcm_data = 0;
    buffer_read_pos = 0;
    buffer_write_pos = 0;

    for(int i=0; i<5; ++i) {
        enabled[i] = false;
        vol[i] = 0;
        decay_count[i] = 0;
        enable_decay[i] = false;
        loop_decay[i] = false;
        decay_vol_lvl[i] = 0;
        duty_cycle[i] = 0;
        sweep_enable[i] = false;
        sweep_shift[i] = 0;
        sweep_dir[i] = 0;
        sweep_rate[i] = 0;
        sweep_count[i] = 0;
        length[i] = 0;
        wavelength_count_reset[i] = 0;
        wave_pos[i] = 0;
    }

    id = 0;
    frame = 0;
    write_pos = 1;

    init();
    generate_arrays();
    
    //Unpause the audio stream
    if(id > 0)
        SDL_PauseAudioDevice(id, false);
}

void apu::clock_quarter() {
    if(enable_linear_cntr && linear_cntr > 0) --linear_cntr;

    //Envelope decay and looping
    for(uint8_t voice = 0; voice < 4; ++voice) {
        if(voice == TRI) continue;
        if(enabled[voice] && enable_decay[voice] && length[voice] > 0) {
            if(decay_count[voice] == 0) {
                --decay_vol_lvl[voice];
                if(decay_vol_lvl[voice] > 15 && loop_decay[voice]) {
                    decay_vol_lvl[voice] = 15;
                }
                else if(decay_vol_lvl[voice] > 15) {
                    decay_vol_lvl[voice] = 0;
                    //enabled[voice] = false;
                    length[voice] = 0;
                }
                else {
                    decay_count[voice] = vol[voice];
                }
                //cout<<"Voice: "<<int(voice)<<" new vol: "<<decay_vol_lvl[voice]<<endl;
            }
            else {
                --decay_count[voice];
            }
        }
    }
}

void apu::clock_half() {
    for(uint8_t voice = 0; voice < 4; ++voice) {
        //clock length counter
        if(enabled[voice] && length[voice] > 0 && !loop_decay[voice] && (voice != TRI || !enable_linear_cntr)) length[voice]--;

        //Wavelength sweep
        if(voice < 2 && enabled[voice] && length[voice] > 0 && sweep_enable[voice]) {
            --sweep_count[voice];
            if(sweep_count[voice] == 0) {
                int sweep_amount = wavelength_count_reset[voice];
                sweep_amount>>=(sweep_shift[voice]);
                if(sweep_dir[voice] == 1) //decrease wavelength
                    sweep_amount *= -1;
                if(wavelength_count_reset[voice] + sweep_amount < 0x7ff && wavelength_count_reset[voice] + sweep_amount > 0x08) {
                    wavelength_count_reset[voice] += sweep_amount;
                    if(sweep_count[voice] == 0) sweep_count[voice] = sweep_rate[voice] + 1;
                }
                else {
                    length[voice] = 0;
                    //enabled[voice] = 0;
                }
            }
        }   
    }

}

void apu::clock_full() {
}

void apu::gen_audio(uint16_t to_gen) { //To be run once per frame, to generate audio
    //cout<<"Gen_audio"<<endl;
    write_pos = !write_pos;
    //assert(to_gen == SAMPLES_PER_FRAME);
    //std::cout<<"apu::gen_audio("<<to_gen<<")"<<std::endl;
    uint16_t reg_audio_sample = SAMPLES_PER_FRAME + 20;
    int write_cycle = 0;
    int write_frame = 0;
    if(writes.size() > 0) { //If there are reg writes left on the queue, find the frame where the next one should apply
        //cout<<writes.size()<<" reg writes in queue"<<endl;
        write_frame = writes.front().first.first;
        //frame = write_frame;
        write_cycle = writes.front().first.second; //Write cycle is in terms of PPU cycle
        reg_audio_sample = uint16_t(double(write_cycle) * double(SAMPLES_PER_FRAME) / double(CLK_PER_FRAME)); //Convert it to which sample the change applies to
        //if(writes.size() > 50) {
            //cout<<"writes.size() = "<<writes.size()<<" next reg at sample: "<<reg_audio_sample<<endl;
        //}
    }

    for(int i = 0; i < SAMPLES_PER_FRAME; ++i) {
        //cout<<"Sample "<<i<<endl;
        //clock the 60Hz, 120Hz, and 240Hz things
        if(i == SAMPLES_PER_FRAME / 4) {
            clock_quarter();
        }
        else if(i == SAMPLES_PER_FRAME / 2) {
            clock_quarter();
            clock_half();
        }
        else if(i == (SAMPLES_PER_FRAME * 3) / 4) {
            clock_quarter();
        }
        else if(i == (SAMPLES_PER_FRAME - 1)) {
            clock_quarter();
            clock_half();
        }
        while(writes.size() > 0 && ((write_frame == frame && reg_audio_sample <= i) || write_frame < frame)) { //Apply Register writes that go here
            //cout<<"Woot, applying a reg write with "<<writes.size()<<"writes remaining"<<endl;
            reg_dequeue();
            if(writes.size() > 0) {
                write_frame = writes.front().first.first;
                write_cycle = writes.front().first.second;
                reg_audio_sample = uint16_t(double(write_cycle) * double(SAMPLES_PER_FRAME) / double(CLK_PER_FRAME));
            }
            else {
                reg_audio_sample = SAMPLES_PER_FRAME + 20; //i.e. don't trigger a dequeue again
            }
        }
        char accum = 0;
        for(int voice = 0; voice < 4; ++voice) {
            //cout<<"Doing voice "<<voice<<endl;
            //Get sample value for each voice, put into appropriate buffer location
            if(enabled[voice] && length[voice] != 0 && (voice != TRI || !enable_linear_cntr || linear_cntr != 0)) {
                //cout<<"getting sample =)"<<endl;
                accum += get_sample(voice);
            }
            //else
            //    cout<<"Not getting sample for voice "<<voice<<endl;
        }
        //if(int(accum) + pcm_data > 255) cout<<"Too big: accum="<<int(accum)<<", pcm="<<pcm_data<<endl;
        //if(pcm_data != 0) cout<<"pcm="<<pcm_data<<endl;
        //cout<<"first: "<<dat.first<<"loc: "<<dat.first*SAMPLES_PER_FRAME<<endl;
        dat.buffer[i+(buffer_write_pos * SAMPLES_PER_FRAME)] = accum + pcm_data;

    }
    //cout<<hex<<val.first<<": 0x"<<val.second.first<<" val: 0x"<<val.second.second<<endl;
    //frame++;
    //if(write_pos) write_pos = 0;
    //else write_pos = 1;
    //cout<<"End of gen_audio"<<endl;
    dat.buffered+=SAMPLES_PER_FRAME;
    buffer_write_pos++;
    buffer_write_pos %= obtained.samples;
    //cout<<"Generate 735"<<endl;
    //cout<<"Write: Frame: "<<frame<<" buffer index: "<<buffer_write_pos<<" samples: "<<buffer_write_pos*SAMPLES_PER_FRAME<<" -> "<<(buffer_write_pos+1)*SAMPLES_PER_FRAME<<endl;
}

//Mem class writes here, and I process the register writes at the end of the frame while generating audio data
void apu::reg_write(int frame, int cycle, int addr, int val) {
    writes.push_back(make_pair(make_pair(frame, cycle), make_pair(addr, val)));
    //cout<<"Pushing ("<<cycle<<", "<<addr<<", "<<val<<")"<<endl;
}

//Read the play status of the channels
//The mem class can probably handle the interrupt info
int  apu::read_status() {
    status_reg a;
    a.sq1_on = (length[SQ1] > 0);
    a.sq2_on = (length[SQ2] > 0);
    a.tri_on = (length[TRI] > 0);
    a.noise_on = (length[NOISE] > 0);
    a.dmc_on = (length[DMC] > 0);
    return a.val;
}

//Process and apply the next register value
void apu::reg_dequeue() {
    if(writes.size() > 0) {
        int addr = writes.front().second.first - 0x4000;
        int chan = addr / 4;
        int reg = addr % 4;
        if(addr == 0x11) reg = 0x11;
        if(addr == 0x15) reg = 0x15;
        if(addr == 0x17) reg = 0x17;
        int val = writes.front().second.second;
        if(reg < 4 && addr > 0x10) writes.pop_front();
        switch(reg) {
            case 0:
                if(chan != TRI) {
                    vol_reg a;
                    a.val = val;
                    vol[chan] = a.vol;
                    enable_decay[chan] = !a.disable_decay;
                    loop_decay[chan] = a.loop_decay;
                    decay_count[chan] = a.vol;
                    //cout<<"Chan: "<<chan<<" Enable decay: "<<enable_decay[chan]<<" loop: "<<a.loop_decay<<" rate: "<<a.vol<<endl;
                    decay_vol_lvl[chan] = 15;
                    if(chan != NOISE) {
                        duty_cycle[chan] = a.duty_cycle;
                    }
                }
                else {
                    lin_ctr_reg a;
                    a.val = val;
                    linear_cntr = a.count;
                    enable_linear_cntr = a.length_disable;
                }
                break;
            case 1: {
                sweep_reg a;
                a.val = val;
                sweep_enable[chan] = a.enable;
                sweep_shift[chan] = a.shift;
                sweep_rate[chan] = a.rate;
                sweep_count[chan] = a.rate + 1;
                sweep_dir[chan] = a.dir;
                }
                break;
            case 2:
                if(chan != NOISE) {
                    freq_len_reg a;
                    a.freq = wavelength_count_reset[chan];
                    a.low = val;
                    wavelength_count_reset[chan] = a.freq;
                }
                else {
                    freq_len_reg a;
                    a.low = val;
                    wavelength_count_reset[chan] = noise_freq(a);
                    wavelength_count[chan] = noise_freq(a);
                    duty_cycle[chan] = a.noise_type;
                }
                break;
            case 3: 
                if(chan != NOISE) {
                    freq_len_reg a;
                    a.freq = wavelength_count_reset[chan];
                    a.high = val;
                    wavelength_count_reset[chan] = a.freq;
                    wavelength_count[chan] = a.freq;
                    length[chan] = play_length(a);
                }
                else {
                    freq_len_reg a;
                    a.high = val;
                    length[chan] = play_length(a);
                    if(chan == SQ1 || chan == SQ2 || chan == NOISE)
                        decay_vol_lvl[chan] = 15;
                }
                break;
            case 0x11:
                pcm_data = val;
                break;
            case 0x15: {
                //cout<<"Set 0x15 to "<<val<<endl;
                status_reg a;
                a.val = val;
                enabled[0] = a.sq1_on;
                enabled[1] = a.sq2_on;
                enabled[2] = a.tri_on;
                enabled[3] = a.noise_on;
                enabled[4] = a.dmc_on;
                pin1_enabled = a.sq1_on + a.sq2_on;
                pin2_enabled = a.tri_on + a.noise_on + a.dmc_on;
                }
                break;
            case 0x17: {
                frame_counter_reg a;
                a.val = val;
                if(a.counter_mode) {
                    clock_quarter();
                    clock_full();
                    framecounter_reset = 111846;
                }
                else {
                    framecounter_reset = 89490;
                }
                enable_irq = !a.irq_disable;
                counter_mode = a.counter_mode;
                framecounter_pos = 0;
                }
                break;
            default:
                cout<<"Not implemented"<<endl;
                break;
        }
        writes.pop_front();
    }
}

//Do the lookup for the actual noise wavelength
const int apu::noise_freq(freq_len_reg val) {
    return noise_freq_table[val.noise_freq];
}

//Convert the data in the struct into a frame count for how long the note should play
const int apu::play_length(freq_len_reg val) {
    if(!(val.bit3 || val.bit7))
        return pal_len_table[val.mid];
    else if(!val.bit3 && val.bit7)
        return ntsc_len_table[val.mid];
    else
        return general_len_table[val.mid + val.bit7*128];
}

//Do the downsampling of audio samples transparently, and provide that to the audio generation function
int apu::get_sample(int voice) {
    if(!enabled[voice] || length[voice] == 0 || wavelength_count_reset[voice] == 0) {
        //cout<<"Returning 0"<<endl;
        //cout<<"Enabled: "<<enabled[voice]<<" wavelength: "<<wavelength_count[voice]<<endl;
        return 0;
    }
    //cout<<"Generating something."<<endl;
    int to_gen = 0;
    float interleave = float(CLK_PER_FRAME * 20) / float(SAMPLE_RATE);
    if(interleave - int(interleave) >= 0.5) to_gen = int(interleave) + 1;
    else to_gen = int(interleave);
    //cout<<"to_gen: "<<to_gen<<endl;
    int accum = 0;
    int vol = 0;
    while(to_gen > 0) {
        //cout<<to_gen<<" left to go!"<<endl;
        int this_one = 0;
        if(wavelength_count[voice] == 0) clock_duty(voice);
        if(wavelength_count[voice] >= to_gen) {
            this_one = to_gen;
        }
        else this_one = wavelength_count[voice];
            
        //cout<<"Doing "<<this_one<<" for this iteration"<<endl;
        switch(voice) {
            case SQ1:
                vol = sq1_duty();
                break;
            case SQ2:
                vol = sq2_duty();
                break;
            case TRI:
                vol = tri_duty();
                break;
            case NOISE:
                vol = noise_duty();
                break;
        }
        //if(vol != 0) cout<<"Vol: "<<vol<<endl;
        to_gen -= this_one;
        wavelength_count[voice] -= this_one;
        accum += (vol * this_one);
        if(wavelength_count[voice] == 0) {
            //clock the channel's duty thingie
            clock_duty(voice);
        }
    }
    //cout<<"Accum: "<<accum<<" returning: "<<accum/41<<endl;
    return accum / 41;
}

void apu::clock_duty(int voice) {
    wavelength_count[voice] = wavelength_count_reset[voice];
    if(voice != NOISE)
        ++wave_pos[voice];
    switch(voice) {
        case SQ1:
        case SQ2:
            wave_pos[voice] %= SQ_LEN;
            break;
        case TRI:
            wave_pos[voice] %= TRI_LEN;
            break;
        case NOISE:
            if(noise_flip_flop) ++wave_pos[voice];
            noise_flip_flop = !noise_flip_flop;
            wave_pos[voice] %= NOISE_LEN[duty_cycle[voice]];
            break;
    }
}

const int apu::sq1_duty() {
    int retval = 0;
    if(enable_decay[SQ1]) retval = SQ_STATES[duty_cycle[SQ1]][wave_pos[SQ1]] * decay_vol_lvl[SQ1];
    else                  retval = SQ_STATES[duty_cycle[SQ1]][wave_pos[SQ1]] * vol[SQ1];
    //cout<<"wave_pos[SQ1]: "<<wave_pos[SQ1]<<" vol: "<<retval<<endl;
    //if(retval > 15) cout<<"SQ1 decay: "<<enable_decay[SQ1]<<" vol: "<<retval<<endl;
    return retval;
}

const int apu::sq2_duty() {
    int retval = 0;
    if(enable_decay[SQ2]) retval = SQ_STATES[duty_cycle[SQ2]][wave_pos[SQ2]] * decay_vol_lvl[SQ2];
    else                  retval = SQ_STATES[duty_cycle[SQ2]][wave_pos[SQ2]] * vol[SQ2];
    //cout<<"wave_pos[SQ2]: "<<wave_pos[SQ2]<<" vol: "<<retval<<endl;
    //if(retval > 15) cout<<"SQ2 decay: "<<enable_decay[SQ2]<<" vol: "<<retval<<endl;
    return retval;
}

const int apu::tri_duty() {
    int retval = TRI_STATES[wave_pos[TRI]];
    //cout<<"wave_pos[TRI]: "<<wave_pos[TRI]<<endl;
    return retval;
}

const int apu::noise_duty() {
    int retval = 0;
    if(enable_decay[NOISE]) retval = noise_states[duty_cycle[NOISE]][wave_pos[NOISE]] * decay_vol_lvl[NOISE];
    else                    retval = noise_states[duty_cycle[NOISE]][wave_pos[NOISE]] * vol[NOISE];
    //cout<<"wave_pos[NOISE]: "<<wave_pos[NOISE]<<endl;
    return retval;
}

apu::~apu() {
    SDL_PauseAudioDevice(id, true);
    SDL_CloseAudioDevice(id);
    //std::cout<<"apu destructor"<<std::endl;
}
