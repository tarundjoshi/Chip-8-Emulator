#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<time.h>
#include "SDL.h"

typedef struct{
    SDL_Window *window;
    SDL_Renderer *renderer;
}sdl_t;
typedef struct {
    uint32_t window_width;
    uint32_t window_height;
    uint32_t fg_color;   // RGBA 8 8 8 8 
    uint32_t bg_color;
    uint32_t scale_factor; // amount to scale pixels by
    uint16_t instructions_per_second; // CPU clock rate
} config_t;

// Emulator states
typedef enum {
    QUIT,
    RUNNING,
    PAUSED,
} emulator_state_t;

// CHIP8 Instruction format
typedef struct {
    uint16_t opcode;
    uint16_t NNN;           // 12 bit address
    uint8_t NN;             // 8 bit constant
    uint8_t N;              // 4 bit constant
    uint8_t X;              // 4 bit register identifier
    uint8_t Y;              // 4 bit register identifier
  

} instruction_t;

// CHIP8 Machine object
typedef struct {
    emulator_state_t state; 
    uint8_t ram[4096];
    bool display[64*32];    // Emulate Original Chip8 resolution pixels ON or OFF pixel 
    uint16_t stack[12];     // subroutines 12 level of stack
    uint16_t *stack_ptr;            // stack pointer
    uint8_t V[16];          // Data registers V0-VF
    uint16_t I;             // Index register
    uint16_t PC;            // Program Counter
    uint8_t delay_timer;    // Decrements at 60 Hz when > 0 
    uint8_t sound_timer;    // Decrements at 60 Hz when > 0
    bool keypad[16];        // Hexadecimal keypad 0x0-0xF
    char *rom_name;         // currently running ROM
    instruction_t inst;     // current instruction
} chip8_t;

#ifdef DEBUG
    #include "debug.h"
#endif

// init chip8 machine
bool init_chip8(chip8_t *chip8, char rom_name[]){
    const uint32_t entry_point = 0x200;  // CHIP8 ROM will be loaded to 0x200
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    // load font 
    memcpy(&chip8->ram[0], font, sizeof(font));

    // Open ROM file
    FILE *rom = fopen(rom_name, "rb");
    if(rom == NULL){
        SDL_Log("Unable to open ROM: %s", rom_name);
        return 0;
    }
    // Check ROM size
    fseek(rom, 0, SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof chip8->ram - entry_point;
    rewind(rom);
    
    if(rom_size > max_size){
        SDL_Log("ROM too large: %s", rom_name);
        fclose(rom);
        return 0;
    }

    fread(&chip8->ram[entry_point], rom_size, 1, rom);

    fclose(rom);
    // set machine defaults 
    chip8->state = RUNNING;
    chip8->PC = entry_point;
    chip8->rom_name = rom_name;
    chip8->stack_ptr = &chip8->stack[0];
    return 1;
}
bool init_sdl(sdl_t *sdl, const config_t config){
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return false;
    }

    // Create a window
    sdl->window = SDL_CreateWindow( 
        "Chip8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config.window_width * config.scale_factor,
        config.window_height * config.scale_factor,
        0
    );
    if(sdl->window == NULL){
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return false;
    }

    sdl->renderer = SDL_CreateRenderer(
        sdl->window, -1, SDL_RENDERER_ACCELERATED
    );
    if(sdl->renderer == NULL){
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        return false;
    }

    return true;
}
// set up emulator config from arguments
bool set_config_from_args(config_t *config, int argc, char *argv[]){
    // set defaults
    *config = (config_t){
        .window_width = 64,         //SDL window height
        .window_height = 32,        //SDL window width
        .fg_color = 0xFFFFFFFF,       //foreground color (WHITE)
        .bg_color = 0x00000000,       //background color
        .scale_factor = 20,         // 1280*640 now
        .instructions_per_second = 500 // CPU clock rate
    };

    return true;

    (void) argc;
    (void) argv;
}
void final_cleanup(sdl_t *sdl){
    SDL_DestroyWindow(sdl->window);
    SDL_DestroyRenderer(sdl->renderer);
    SDL_Quit();
}

// clear screen to background color
void clear_screen(sdl_t *sdl, config_t config){
    const uint8_t r  = config.bg_color >> 24;
    const uint8_t g  = config.bg_color >> 16;
    const uint8_t b  = config.bg_color >> 8;
    const uint8_t a  = config.bg_color >> 0;

    SDL_SetRenderDrawColor(sdl->renderer, r, g, b, a);
    SDL_RenderClear(sdl->renderer);
}
// update screen with current state

void update_screen(sdl_t *sdl, const config_t config, chip8_t *chip8){

    // (void) chip8;
    // (void) config;
    SDL_Rect rect = {0, 0, config.scale_factor, config.scale_factor};
    // colors to draw
    const uint8_t fg_r  = config.fg_color >> 24;
    const uint8_t fg_g  = config.fg_color >> 16;
    const uint8_t fg_b  = config.fg_color >> 8;
    const uint8_t fg_a  = config.fg_color >> 0;

    const uint8_t bg_r  = config.bg_color >> 24;
    const uint8_t bg_g  = config.bg_color >> 16;
    const uint8_t bg_b  = config.bg_color >> 8;
    const uint8_t bg_a  = config.bg_color >> 0;

    // loop through display pixels, draw a rectangle per pixel to the SDL window
    for(u_int32_t i = 0; i < sizeof chip8->display; i++){
        rect.x = i % config.window_width * config.scale_factor;
        rect.y = i / config.window_width * config.scale_factor;
        if(chip8->display[i]){
            SDL_SetRenderDrawColor(sdl->renderer, fg_r, fg_g, fg_b, fg_a);
            SDL_RenderFillRect(sdl->renderer, &rect);

            // for pixelated effect
            SDL_SetRenderDrawColor(sdl->renderer, bg_r, bg_g, bg_b, bg_a);
            SDL_RenderDrawRect(sdl->renderer, &rect);
        }
        else{
            SDL_SetRenderDrawColor(sdl->renderer, bg_r, bg_g, bg_b, bg_a);
            SDL_RenderFillRect(sdl->renderer, &rect);
        }
    }
    SDL_RenderPresent(sdl->renderer);
}

// handle user input
// chip8 keypad     mapped to 
// 123D             1234
// 456C             qwert        
// 789B             asdf
// A0BF             zxcv
void handle_input(chip8_t *chip8){
    SDL_Event event;
    
    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT:
                chip8->state = QUIT;
                return;
            case SDL_KEYDOWN:                   // check for key press
                switch(event.key.keysym.sym){   // switch on key pressed
                    case SDLK_ESCAPE:
                        // escape key : Exit 
                        chip8->state = QUIT;
                        return;
                    case SDLK_SPACE:
                        // space key : toggle pause
                        if(chip8->state == RUNNING){
                            chip8->state = PAUSED;
                            printf("Paused\n");
                        }
                        else chip8->state = RUNNING;
                        return;
                    case SDLK_1: chip8->keypad[0x1] = true; break;
                    case SDLK_2: chip8->keypad[0x2] = true; break;
                    case SDLK_3: chip8->keypad[0x3] = true; break;
                    case SDLK_4: chip8->keypad[0xC] = true; break;

                    case SDLK_q: chip8->keypad[0x4] = true; break;
                    case SDLK_w: chip8->keypad[0x5] = true; break;
                    case SDLK_e: chip8->keypad[0x6] = true; break;
                    case SDLK_r: chip8->keypad[0xD] = true; break;

                    case SDLK_a: chip8->keypad[0x7] = true; break;
                    case SDLK_s: chip8->keypad[0x8] = true; break;
                    case SDLK_d: chip8->keypad[0x9] = true; break;
                    case SDLK_f: chip8->keypad[0xE] = true; break;

                    case SDLK_z: chip8->keypad[0xA] = true; break;
                    case SDLK_x: chip8->keypad[0x0] = true; break;
                    case SDLK_c: chip8->keypad[0xB] = true; break;
                    case SDLK_v: chip8->keypad[0xF] = true; break;

                    default:
                        break;
                }
                break;
            case SDL_KEYUP:
                switch(event.key.keysym.sym){
                    case SDLK_1: chip8->keypad[0x1] = false; break;
                    case SDLK_2: chip8->keypad[0x2] = false; break;
                    case SDLK_3: chip8->keypad[0x3] = false; break;
                    case SDLK_4: chip8->keypad[0xC] = false; break;

                    case SDLK_q: chip8->keypad[0x4] = false; break;
                    case SDLK_w: chip8->keypad[0x5] = false; break;
                    case SDLK_e: chip8->keypad[0x6] = false; break;
                    case SDLK_r: chip8->keypad[0xD] = false; break;
                    
                    case SDLK_a: chip8->keypad[0x7] = false; break;
                    case SDLK_s: chip8->keypad[0x8] = false; break;
                    case SDLK_d: chip8->keypad[0x9] = false; break;
                    case SDLK_f: chip8->keypad[0xE] = false; break;

                    case SDLK_z: chip8->keypad[0xA] = false; break;
                    case SDLK_x: chip8->keypad[0x0] = false; break;
                    case SDLK_c: chip8->keypad[0xB] = false; break;
                    case SDLK_v: chip8->keypad[0xF] = false; break;
                }
                break;

            default:
                break;

        }
    }
}
// Emulate 1 Chip8 instruction
void emulate_instruction(chip8_t *chip8, config_t config){
    // fetch instruction from ram
    chip8->inst.opcode = (chip8->ram[chip8->PC] << 8) | chip8->ram[chip8->PC + 1];
    chip8->PC += 2;
    // fill out current instruction format
    chip8->inst.NNN = chip8->inst.opcode & 0x0FFF;
    chip8->inst.NN = chip8->inst.opcode & 0x0FF;
    chip8->inst.N = chip8->inst.opcode & 0x0F;
    chip8->inst.X = (chip8->inst.opcode >> 8) & 0x0F;
    chip8->inst.Y = (chip8->inst.opcode >> 4) & 0x0F;

#ifdef DEBUG
    print_debug_info(chip8);
#endif


    // decode instruction
    switch ((chip8->inst.opcode >> 12) & 0x0F){
        case 0x0:
            if(chip8->inst.NN == 0xE0){
                // clear screen (0x00E0)
                memset(&chip8->display[0], 0, sizeof(chip8->display));
            }
            else if(chip8->inst.NN == 0xEE){
                // return from subroutine (0x00EE)
                // set pc to last address on subroutine stack ("pop" from stack)
                chip8->PC = *--chip8->stack_ptr;
            }
            break;

        case 0x1:
            // jump to address (0x1NNN)
            chip8->PC = chip8->inst.NNN;
            break;
        
        case 0x2:
            // call subroutine (0x2NNN)
            // push current address to return to on subroutine stack
            // set pc to subroutine address so that next opcode is gotten from there
            *chip8->stack_ptr++ = chip8->PC; 
            chip8->PC = chip8->inst.NNN;
            break;
        
        case 0x3:
            // skip next instruction if V[x] == NN (0x3XNN)
            if(chip8->V[chip8->inst.X] == chip8->inst.NN){
                chip8->PC += 2;
            }
            break;

        case 0x4:
            // skip next instruction if V[x] != NN (0x4XNN)
            if(chip8->V[chip8->inst.X] != chip8->inst.NN){
                chip8->PC += 2;
            }
            break;
        
        case 0x5:
            // skip next instruction if V[x] == V[y] (0x5XY0)
            
            if(chip8->V[chip8->inst.X] == chip8->V[chip8->inst.Y]){
                chip8->PC += 2;
            }
            break;
        
        case 0x6:
            // set V[x] = NN  (0x6XNN)
            chip8->V[chip8->inst.X] = chip8->inst.NN;
            break;

        case 0x7:
            // v[x] += NN
            chip8->V[chip8->inst.X] += chip8->inst.NN;
            break;

        case 0x8:
            switch(chip8->inst.N){
                case 0x0:
                    // set V[x] = V[y] (0x8XY0)
                    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y];
                    break;
                case 0x1:
                    // set V[x] = V[x] | V[y] (0x8XY1)
                    chip8->V[chip8->inst.X] |= chip8->V[chip8->inst.Y];
                    break;
                case 0x2:
                    // set V[x] = V[x] & V[y] (0x8XY2)
                    chip8->V[chip8->inst.X] &= chip8->V[chip8->inst.Y];
                    break;
                case 0x3:
                    // set V[x] = V[x] ^ V[y] (0x8XY3)
                    chip8->V[chip8->inst.X] ^= chip8->V[chip8->inst.Y];
                    break;
                case 0x4:
                    // set V[x] = V[x] + V[y] (0x8XY4) set v[f] = 1 if carry
                    // if((uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255)
                    //     chip8->V[0xF] = 1;

                    chip8->V[0xF] = (chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255 ? 1 : 0;
                    
                    chip8->V[chip8->inst.X] += chip8->V[chip8->inst.Y];
                    break;
                case 0x5:
                    // set V[x] = V[x] - V[y] (0x8XY5) set v[f] = 1 if no borrow (positive result)
                    // if(chip8->V[chip8->inst.X] >= chip8->V[chip8->inst.Y])
                    //     chip8->V[0xF] = 1;

                    chip8->V[0xF] = chip8->V[chip8->inst.X] >= chip8->V[chip8->inst.Y] ? 1 : 0;
                    
                    chip8->V[chip8->inst.X] -= chip8->V[chip8->inst.Y];
                    break;
                case 0x6:
                    // set V[x] = V[x] >> 1 (0x8XY6) set v[f] = least significant bit of V[x]
                    chip8->V[0xF] = chip8->V[chip8->inst.X] & 1;
                    chip8->V[chip8->inst.X] >>= 1;
                    break;
                case 0x7:
                    // set V[x] = V[y] - V[x] (0x8XY7) set v[f] = 1 if no borrow (positive result)
                    // if(chip8->V[chip8->inst.Y] >= chip8->V[chip8->inst.X])
                    //     chip8->V[0xF] = 1;

                    chip8->V[0xF] = chip8->V[chip8->inst.Y] >= chip8->V[chip8->inst.X] ? 1 : 0;

                    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] - chip8->V[chip8->inst.X];
                    break;
                case 0xE:
                    // set V[x] = V[x] << 1 (0x8XYE) set v[f] = most significant bit of V[x]
                    chip8->V[0xF] = (chip8->V[chip8->inst.X] & 0x80) >> 7;
                    chip8->V[chip8->inst.X] <<= 1;
                    break;
                
                default:
                    break;
            }
            break;

        case 0x9:
            // skip next instruction if V[x] != V[y] (0x9XY0)
            if(chip8->V[chip8->inst.X] != chip8->V[chip8->inst.Y])
                chip8->PC += 2;
            break;

        case 0xA:
            // set index register (0xANNN)
            chip8->I = chip8->inst.NNN;
            break;
        
        case 0xB:
            // jump to address NNN + V[0] (0xBNNN)
            chip8->PC = chip8->inst.NNN + chip8->V[0];
            break;

        case 0xC:
            // set V[x] = random byte AND NN (0xCXNN)
            chip8->V[chip8->inst.X] = (rand() % 256) & chip8->inst.NN;
            break;
            
        case 0xD:
            // draw sprite (0xDXYN) Draw N-Height at coords X, Y : Read from memory loc I;
            // screen pixels are XOR'd with sprite pixels
            // VF (Carry Flag) is set if any pixels were erased

            uint8_t X_coord = chip8->V[chip8->inst.X] % config.window_width;
            uint8_t Y_coord = chip8->V[chip8->inst.Y] % config.window_height;
            const uint8_t orig_X = X_coord;

            chip8->V[0xF] = 0; // set VF to 0

            // loop over N rows of sprite
            for(uint8_t i = 0; i < chip8->inst.N; i++){
                uint8_t sprite = chip8->ram[chip8->I + i];
                X_coord = orig_X;   // reset x for next row

                for(int8_t j = 7 ; j >= 0 ; j--){
                    // condition to set carry flag
                    bool *pixel = &chip8->display[(Y_coord) * config.window_width + (X_coord)];
                    const bool sprite_bit = sprite & (1 << j);
                    if((sprite_bit) && *pixel){
                        
                        chip8->V[0xF] = 1;
                    }

                    *pixel ^= sprite_bit;

                    if(++X_coord >= config.window_width){
                        break;
                    }
                }

                if(++Y_coord >= config.window_height){
                    break;
                }
            }

            break;

        case 0xE:
            if(chip8->inst.NN == 0x9E){
                // skip next instruction if key with the value of V[x] is pressed (0xEX9E)
                if(chip8->keypad[chip8->V[chip8->inst.X]]){
                    chip8->PC += 2;
                }
            }
            else if(chip8->inst.NN == 0xA1){
                // skip next instruction if key with the value of V[x] is not pressed (0xEXA1)
                if(!chip8->keypad[chip8->V[chip8->inst.X]]){
                    chip8->PC += 2;
                }
            }
            break;

        case 0xF:
            switch(chip8->inst.NN){
                case 0x0A:
                   // wait for keypress and store value in V[x] (0xFX0A)
                   bool key_pressed = false;
                   for(uint8_t i = 0; i < sizeof chip8->keypad; i++){
                       if(chip8->keypad[i]){
                           chip8->V[chip8->inst.X] = i;
                           key_pressed = true;
                           break;
                       }
                   }
                   // if no key pressed, set program counter to previous address (again wait for input)
                   if(!key_pressed){
                       chip8->PC -= 2;
                   }

                   break;

                case 0x1E:
                    // add V[x] to I (0xFX1E)
                    chip8->I += chip8->V[chip8->inst.X];
                    break;

                case 0x07:
                    // set V[x] = delay timer value (0xFX07)
                    chip8->V[chip8->inst.X] = chip8->delay_timer;
                    break;

                case 0x15:
                    // set delay timer = V[x] (0xFX15)
                    chip8->delay_timer = chip8->V[chip8->inst.X];
                    break;
                
                case 0x18:
                    // set sound timer = V[x] (0xFX18)
                    chip8->sound_timer = chip8->V[chip8->inst.X];
                    break;
                
                case 0x29:
                    // set I = location of sprite for char V[x] (0xFX29)
                    chip8->I = chip8->V[chip8->inst.X] * 5;
                    break;

                case 0x33:
                    // store BCD representation of V[x] in memory locations I, I+1, I+2 (0xFX33)
                    chip8->ram[chip8->I] = chip8->V[chip8->inst.X] / 100;
                    chip8->ram[chip8->I+1] = (chip8->V[chip8->inst.X] / 10) % 10;
                    chip8->ram[chip8->I+2] = chip8->V[chip8->inst.X] % 10;
                    break;

                case 0x55:
                    // store registers V0 through V[x] in memory starting at location I (0xFX55)
                    // SCHIP does not incrememnt I, but CHIP-8 does
                    for(uint8_t i = 0; i <= chip8->inst.X; i++){
                        chip8->ram[chip8->I + i] = chip8->V[i];
                    }
                    // chip8->I = chip8->I + chip8->inst.X + 1;
                    break;

                case 0x65:
                    // load registers V0 through V[x] from memory starting at location I (0xFX65)
                    // SCHIP does not incrememnt I, but CHIP-8 does
                    for(uint8_t i = 0; i <= chip8->inst.X; i++){
                        chip8->V[i] = chip8->ram[chip8->I + i];
                    }
                    // chip8->I = chip8->I + chip8->inst.X + 1;
                    break;
                
                default:
                    break;
            }
        
        default :
            break;
    }
}
void update_timers(chip8_t *chip8){
    if(chip8->delay_timer > 0) chip8->delay_timer--;
    if(chip8->sound_timer > 0) chip8->sound_timer--;
    //TODO: play sound
}
int main(int argc, char *argv[]){
    // Defualt usage message
    if(argc < 2){
        fprintf(stderr, "Usage: %s <ROM>\n", argv[0]);
        return 0;
    }

    srand(time(NULL));

    // initialize emulator config
    config_t config = {0};
    if(!set_config_from_args(&config, argc, argv)) exit(0);


    // initialize SDL
    sdl_t sdl = {0};
    if(!init_sdl(&sdl, config)) exit(0);

    // initialise CHIP8 machine
    chip8_t chip8 = {0};
    char *rom_name = argv[1];
    if(!init_chip8(&chip8, rom_name)) exit(0);

    // initial screen clear
    clear_screen(&sdl, config);

    // Main emulator loop
    while(chip8.state != QUIT){
        handle_input(&chip8);

        if (chip8.state == PAUSED) continue;


        uint64_t start = SDL_GetPerformanceCounter();
        //Emulate instructions in one frame
        for(uint32_t i = 0; i < config.instructions_per_second/60; i++){
            emulate_instruction(&chip8, config);
        }
        uint64_t end = SDL_GetPerformanceCounter();
        const double time_elapsed = (end - start)*1000 / (double) SDL_GetPerformanceFrequency();

        // Delay for 60Hz approximately ~ 16.6 ms
        SDL_Delay(16.67f > time_elapsed ? 16.67f - time_elapsed : 0);

        // Update window 
        update_screen(&sdl, config, &chip8);

        // Update timers
        update_timers(&chip8);
    }

    // Final cleanup
    final_cleanup(&sdl);

    exit(0);
}