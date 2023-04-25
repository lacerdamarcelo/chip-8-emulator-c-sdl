#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#define IS_SUPER_CHIP 1
#define MEMORY_SIZE 4096
#define STACK_SIZE 16
#define NUMBER_GENERAL_PURPOSE_REGISTERS 16
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define DELAY 2
#define SCREEN_SCALING_FACTOR 20
// In the original CHIP-8, the interpreter was stored between 0x0000 and 0x01FF.
#define PROGRAM_STARTING_ADDRESS 0x0200
// Just following a convention.
#define FONT_STARTING_ADDRESS 0x0050

unsigned short pop_stack(unsigned short *stack, unsigned char *stack_top_pointer){
    if(*stack_top_pointer > 0){
        *stack_top_pointer = *stack_top_pointer - 1;
        unsigned short data = stack[*stack_top_pointer];
        return data;
    }
    printf("Tried to pop from an empty stack.\n");
    exit(255);
}

void push_stack(unsigned short *stack, unsigned short value, unsigned char *stack_top_pointer){
    if(*stack_top_pointer < STACK_SIZE - 1){
        stack[*stack_top_pointer] = value;
        *stack_top_pointer = *stack_top_pointer + 1;
        return;
    }
    printf("Tried to push new data into a full stack.\n");
    exit(255);
}

void update_timers(unsigned char *delay_timer, unsigned char *sound_timer){
    *delay_timer = *delay_timer - 1;
    *sound_timer = *sound_timer - 1;
    if(*delay_timer < 0){
        *delay_timer = 0xFF;
    }
    if(*sound_timer < 0){
        *sound_timer = 0xFF;
    }
    if(*sound_timer > 0){
        // This sound is annoying, so I disabled it.
        //putchar('\a');
    }
}

void get_keyboard_input(SDL_Event event, unsigned char *pressed_keys){
    if (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_KEYDOWN) {
            pressed_keys[0x10] = 0x00;
            // Following COSMAC VIP keypad positions, starting at 1 in the QWERTY keyboard.
            switch(event.key.keysym.sym){
                case SDLK_1:
                    pressed_keys[0x01] = 0x01;
                    break;
                case SDLK_2:
                    pressed_keys[0x02] = 0x01;
                    break;
                case SDLK_3:
                    pressed_keys[0x03] = 0x01;
                    break;
                case SDLK_4:
                    pressed_keys[0x0C] = 0x01;
                    break;
                case SDLK_q:
                    pressed_keys[0x04] = 0x01;
                    break;
                case SDLK_w:
                    pressed_keys[0x05] = 0x01;
                    break;
                case SDLK_e:
                    pressed_keys[0x06] = 0x01;
                    break;
                case SDLK_r:
                    pressed_keys[0x0D] = 0x01;
                    break;
                case SDLK_a:
                    pressed_keys[0x07] = 0x01;
                    break;
                case SDLK_s:
                    pressed_keys[0x08] = 0x01;
                    break;
                case SDLK_d:
                    pressed_keys[0x09] = 0x01;
                    break;
                case SDLK_f:
                    pressed_keys[0x0E] = 0x01;
                    break;
                case SDLK_z:
                    pressed_keys[0x0A] = 0x01;
                    break;
                case SDLK_x:
                    pressed_keys[0x00] = 0x01;
                    break;
                case SDLK_c:
                    pressed_keys[0x0B] = 0x01;
                    break;
                case SDLK_v:
                    pressed_keys[0x0F] = 0x01;
                    break;
            }
        } else if (event.type == SDL_KEYUP) {
            pressed_keys[0x10] = 0x00;
            // Following COSMAC VIP keypad positions, starting at 1 in the QWERTY keyboard.
            switch(event.key.keysym.sym){
                case SDLK_1:
                    pressed_keys[0x01] = 0x00;
                    break;
                case SDLK_2:
                    pressed_keys[0x02] = 0x00;
                    break;
                case SDLK_3:
                    pressed_keys[0x03] = 0x00;
                    break;
                case SDLK_4:
                    pressed_keys[0x0C] = 0x00;
                    break;
                case SDLK_q:
                    pressed_keys[0x04] = 0x00;
                    break;
                case SDLK_w:
                    pressed_keys[0x05] = 0x00;
                    break;
                case SDLK_e:
                    pressed_keys[0x06] = 0x00;
                    break;
                case SDLK_r:
                    pressed_keys[0x0D] = 0x00;
                    break;
                case SDLK_a:
                    pressed_keys[0x07] = 0x00;
                    break;
                case SDLK_s:
                    pressed_keys[0x08] = 0x00;
                    break;
                case SDLK_d:
                    pressed_keys[0x09] = 0x00;
                    break;
                case SDLK_f:
                    pressed_keys[0x0E] = 0x00;
                    break;
                case SDLK_z:
                    pressed_keys[0x0A] = 0x00;
                    break;
                case SDLK_x:
                    pressed_keys[0x00] = 0x00;
                    break;
                case SDLK_c:
                    pressed_keys[0x0B] = 0x00;
                    break;
                case SDLK_v:
                    pressed_keys[0x0F] = 0x00;
                    break;
            }
        } else if( event.type == SDL_QUIT )
            pressed_keys[0x10] = 0x01;
    }
}

void clean_screen_pixels(unsigned int *screen_pixels){
    for(int i=0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++){
        screen_pixels[i] = 0x00000000;
    } 
}

void update_screen_pixels(unsigned char *memory, unsigned char x_position, unsigned char y_position,
                          unsigned short sprite_address, unsigned char sprite_height, unsigned int *screen_pixels,
                          unsigned char *general_purpose_registers){
    unsigned char sprite_row_data;
    unsigned char mask;
    unsigned char pixel_value;
    unsigned char pixel_position_x;
    unsigned char pixel_position_y;

    general_purpose_registers[0x0F] = 0x00;
    for(unsigned char sprite_height_index = 0; sprite_height_index < sprite_height; sprite_height_index++){
        sprite_row_data = memory[sprite_address + sprite_height_index];
        for(unsigned char sprite_width_index = 0; sprite_width_index < 8; sprite_width_index++){
            mask = 0x01 << (7 - sprite_width_index);
            pixel_value = (sprite_row_data & mask) >> (7 - sprite_width_index);
            pixel_position_x = (x_position % SCREEN_WIDTH) + sprite_width_index;
            pixel_position_y = (y_position % SCREEN_HEIGHT) + sprite_height_index;
            if((pixel_position_x < SCREEN_WIDTH) && (pixel_position_y < SCREEN_HEIGHT) && (pixel_position_x >= 0) && (pixel_position_y >= 0)){
                if(pixel_value == 1){
                    if(screen_pixels[pixel_position_y * SCREEN_WIDTH + pixel_position_x] == 0xFFFFFFFF)
                        general_purpose_registers[0x0F] = 0x01;
                    screen_pixels[pixel_position_y * SCREEN_WIDTH + pixel_position_x] = ~screen_pixels[pixel_position_y * SCREEN_WIDTH + pixel_position_x];
                }
            }
        }
    }
}

void decode_run(unsigned short instruction,
                unsigned short *program_counter,
                unsigned short *stack,
                unsigned char *stack_top_pointer,
                unsigned char *general_purpose_registers,
                unsigned short *index_register,
                unsigned int *screen_pixels,
                unsigned char *memory,
                unsigned char *pressed_keys,
                unsigned char *delay_timer,
                unsigned char *sound_timer,
                int is_super_chip){

    unsigned char register_number_1;
    unsigned char register_number_2;
    unsigned char value_1;
    unsigned char value_2;
    unsigned char sprite_height;
    char shifted_out_bit;
    unsigned short final_address;
    unsigned char number_1;
    unsigned char number_2;
    unsigned char number_3;
    short sum;
    
    switch(instruction & 0xF000){
        case 0x0000:
            switch(instruction){
                case 0x00E0:
                    clean_screen_pixels(screen_pixels);
                    break;
                case 0x00EE:
                    // Returning from subroutine.
                    *program_counter = pop_stack(stack, stack_top_pointer);
                    break;
            }
            break;
        case 0x1000:
            // Jump
            *program_counter = instruction & 0x0FFF;
            break;
        case 0x2000:
            // Calling subroutine.
            push_stack(stack, *program_counter, stack_top_pointer);
            *program_counter = instruction & 0x0FFF; 
            break;
        case 0x3000:
            // Skip if register is equal to value.
            register_number_1 = (instruction & 0x0F00) >> 8;
            if(general_purpose_registers[register_number_1] == (instruction & 0x00FF))
                *program_counter = *program_counter + 2;
            break; 
        case 0x4000:
            // Skip if register is NOT equal to value.
            register_number_1 = (instruction & 0x0F00) >> 8;
            if(general_purpose_registers[register_number_1] != (instruction & 0x00FF))
                *program_counter = *program_counter + 2;
            break; 
        case 0x5000:
            if((instruction & 0x000F) == 0x0000){
                // Skip if register is equal to register.
                register_number_1 = (instruction & 0x0F00) >> 8;
                register_number_2 = (instruction & 0x00F0) >> 4;
                if(general_purpose_registers[register_number_1] == general_purpose_registers[register_number_2])
                    *program_counter = *program_counter + 2;
            }
            break;
        case 0x6000:
            // Set value to register.
            register_number_1 = (instruction & 0x0F00) >> 8;
            general_purpose_registers[register_number_1] = instruction & 0x00FF;
            break;
        case 0x7000:
            // Add value to register.
            register_number_1 = (instruction & 0x0F00) >> 8;
            general_purpose_registers[register_number_1] += instruction & 0x00FF;
            break;
        case 0x8000:
            switch(instruction & 0x000F){
                case 0x0000:
                    // Set VX with the value in VY.
                    register_number_1 = (instruction & 0x0F00) >> 8;
                    register_number_2 = (instruction & 0x00F0) >> 4; 
                    general_purpose_registers[register_number_1] = general_purpose_registers[register_number_2];
                    break;
                case 0x0001:
                    // Bitwise OR between VX and VY.
                    register_number_1 = (instruction & 0x0F00) >> 8;
                    register_number_2 = (instruction & 0x00F0) >> 4;
                    value_1 = general_purpose_registers[register_number_1];
                    value_2 = general_purpose_registers[register_number_2];
                    general_purpose_registers[register_number_1] = value_1 | value_2;
                    break;
                case 0x0002:
                    // Bitwise AND between VX and VY.
                    register_number_1 = (instruction & 0x0F00) >> 8;
                    register_number_2 = (instruction & 0x00F0) >> 4;
                    value_1 = general_purpose_registers[register_number_1];
                    value_2 = general_purpose_registers[register_number_2];
                    general_purpose_registers[register_number_1] = value_1 & value_2;
                    break;
                case 0x0003:
                    // Bitwise XOR between VX and BY.
                    register_number_1 = (instruction & 0x0F00) >> 8;
                    register_number_2 = (instruction & 0x00F0) >> 4;
                    value_1 = general_purpose_registers[register_number_1];
                    value_2 = general_purpose_registers[register_number_2];
                    general_purpose_registers[register_number_1] = value_1 ^ value_2;
                    break;
                case 0x0004:
                    // Sum between VX and VY with carry flag (VF).
                    register_number_1 = (instruction & 0x0F00) >> 8;
                    register_number_2 = (instruction & 0x00F0) >> 4;
                    value_1 = general_purpose_registers[register_number_1];
                    value_2 = general_purpose_registers[register_number_2];
                    sum = value_1 + value_2; 
                    if(sum > 0xFF)
                        general_purpose_registers[0x0F] = 0x01;
                    else
                        general_purpose_registers[0x0F] = 0x00;
                    general_purpose_registers[register_number_1] = value_1 + value_2; 
                    break;
                case 0x0005:
                    register_number_1 = (instruction & 0x0F00) >> 8;
                    register_number_2 = (instruction & 0x00F0) >> 4;
                    value_1 = general_purpose_registers[register_number_1];
                    value_2 = general_purpose_registers[register_number_2];
                    if(value_1 >= value_2)
                        general_purpose_registers[0x0F] = 0x01;
                    else
                        general_purpose_registers[0x0F] = 0x00;
                    general_purpose_registers[register_number_1] = value_1 - value_2; 
                    break;
                case 0x0006:
                    register_number_1 = (instruction & 0x0F00) >> 8;
                    register_number_2 = (instruction & 0x00F0) >> 4;
                    if(is_super_chip == 0)
                        general_purpose_registers[register_number_1] = general_purpose_registers[register_number_2];
                    shifted_out_bit = general_purpose_registers[register_number_1] & 0b00000001;
                    general_purpose_registers[0x0F] = shifted_out_bit;
                    general_purpose_registers[register_number_1] = general_purpose_registers[register_number_1] >> 1; 
                    break;
                case 0x0007:
                    register_number_1 = (instruction & 0x0F00) >> 8;
                    register_number_2 = (instruction & 0x00F0) >> 4;
                    value_1 = general_purpose_registers[register_number_1];
                    value_2 = general_purpose_registers[register_number_2];
                    if(value_2 >= value_1)
                        general_purpose_registers[0x0F] = 0x01;
                    else
                        general_purpose_registers[0x0F] = 0x00;
                    general_purpose_registers[register_number_1] = value_2 - value_1;
                    break;
                case 0x000E:
                    register_number_1 = (instruction & 0x0F00) >> 8;
                    register_number_2 = (instruction & 0x00F0) >> 4;
                    if(is_super_chip == 0)
                        general_purpose_registers[register_number_1] = general_purpose_registers[register_number_2];
                    shifted_out_bit = (general_purpose_registers[register_number_1] & 0b10000000) >> 7;
                    general_purpose_registers[0x0F] = shifted_out_bit;
                    general_purpose_registers[register_number_1] = general_purpose_registers[register_number_1] << 1; 
                    break;
            }
            break;
        case 0x9000:
            if((instruction & 0x000F) == 0x0000){
                // Skip if register is NOT equal to register.
                register_number_1 = (instruction & 0x0F00) >> 8;
                register_number_2 = (instruction & 0x00F0) >> 4;
                if(general_purpose_registers[register_number_1] != general_purpose_registers[register_number_2])
                    *program_counter = *program_counter + 2;
            }
            break;
        case 0xA000:
            // Set value to index register.
            *index_register = instruction & 0x0FFF;
            break;
        case 0xB000:
            // Jump with offset
            if(is_super_chip == 0)
                *program_counter = (instruction & 0x0FFF) + general_purpose_registers[0x00];
            else{
                register_number_1 = (instruction & 0x0F00) >> 8;
                *program_counter = (instruction & 0x0FFF) + general_purpose_registers[register_number_1];
            }
            break;
        case 0xC000:
            // Random number
            register_number_1 = (instruction & 0x0F00) >> 8;
            value_1 = instruction & 0x00FF;
            general_purpose_registers[register_number_1] = (rand() % 0x100) & value_1;
            break;
        case 0xD000:
            // Display
            value_1 = general_purpose_registers[(instruction & 0x0F00) >> 8];
            value_2 = general_purpose_registers[(instruction & 0x00F0) >> 4];
            sprite_height = instruction & 0x000F;
            update_screen_pixels(memory, value_1, value_2, *index_register, sprite_height, screen_pixels,
                                 general_purpose_registers);
            break;
        case 0xE000:
            // Skip if key
            register_number_1 = (instruction & 0x0F00) >> 8;
            switch(instruction & 0x00FF){
                case 0x009E:
                    if(pressed_keys[general_purpose_registers[register_number_1]] == 0x01)
                        *program_counter = *program_counter + 2;
                    break;
                case 0x00A1:
                    if(pressed_keys[general_purpose_registers[register_number_1]] == 0x00)
                        *program_counter = *program_counter + 2;
                    break;
            }
            break;
        case 0xF000:
            register_number_1 = (instruction & 0x0F00) >> 8;
            switch(instruction & 0x00FF){
                case 0x0007:
                    general_purpose_registers[register_number_1] = *delay_timer;
                    break;
                case 0x000A:
                    for(unsigned char i=0; i<17; i++){
                        if(pressed_keys[i] == 0x01){
                            general_purpose_registers[register_number_1] = i;
                            break;
                        }
                    }
                    *program_counter = *program_counter - 2;
                    break;
                case 0x0015:
                    *delay_timer = general_purpose_registers[register_number_1];
                    break;
                case 0x0018:
                    *sound_timer = general_purpose_registers[register_number_1];
                    break;
                case 0x001E:
                    final_address = *index_register + general_purpose_registers[register_number_1];
                    if(is_super_chip != 0){
                        if(final_address > 0x0FFF)
                            general_purpose_registers[0x0F] = 0x01;
                        else
                            general_purpose_registers[0x0F] = 0x00; 
                    }
                    *index_register = final_address % 0x1000;
                    break;
                case 0x0029:
                    // Remember that the font height is 5.
                    *index_register = FONT_STARTING_ADDRESS + (general_purpose_registers[register_number_1] & 0x0F) * 5;
                    break;
                case 0x0033:
                    value_1 = general_purpose_registers[register_number_1];
                    number_1 = value_1 / 100;
                    number_2 = (value_1 % 100) / 10;
                    number_3 = value_1 % 10;
                    memory[*index_register] = number_1;
                    memory[*index_register + 1] = number_2;
                    memory[*index_register + 2] = number_3;
                    break;
                case 0x0055:
                    // Copying array from registers to memory.
                    for(int i = 0; i <= register_number_1; i++)
                        memory[*index_register + i] = general_purpose_registers[i];
                    if(is_super_chip == 0)
                        *index_register = *index_register + register_number_1 + 1;
                    break;
                case 0x0065:
                    // Copying array from memory to registers.
                    for(int i = 0; i <= register_number_1; i++)
                        general_purpose_registers[i] = memory[*index_register + i];
                    if(is_super_chip == 0)
                        *index_register = *index_register + register_number_1 + 1;
                    break;
            }
            break;
    }
}

unsigned short fetch_instruction(unsigned char *memory, unsigned short *program_counter){
    unsigned short instruction = (memory[*program_counter] << 8) + memory[*program_counter + 1];
    *program_counter = *program_counter + 2;
    return instruction;
}

void init_font(unsigned char *memory){
    unsigned char characters[80] = {0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
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
    for(unsigned char i = 0; i < 80; i++)
        memory[FONT_STARTING_ADDRESS + i] = characters[i];
}

void init_memory(unsigned char *memory, unsigned short *stack, unsigned char *general_purpose_registers){
    for(short i = 0; i < MEMORY_SIZE; i++)
        memory[i] = 0x00;
    init_font(memory);
    for(short i = 0; i < STACK_SIZE; i++)
        stack[i] = 0x0000;
    for(short i = 0; i < NUMBER_GENERAL_PURPOSE_REGISTERS; i++)
        general_purpose_registers[i] = 0x00; 
}

void load_rom(unsigned char *memory, char *file_name){
    FILE *file_pointer = fopen(file_name, "rb");
    unsigned char data;
    unsigned short memory_pointer = 0x0000;
    while(!feof(file_pointer)){
        fread(&data, 1, 1, file_pointer);
        memory[PROGRAM_STARTING_ADDRESS + memory_pointer ] = data;
        memory_pointer++;
    }
    fclose(file_pointer);
}

int main(int argc, char **argv){
    srand(time(NULL));

    // Initializing SDL stuff.
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                           SCREEN_WIDTH * SCREEN_SCALING_FACTOR, SCREEN_HEIGHT * SCREEN_SCALING_FACTOR, 0);
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Event event;
    int quit = 0;
    
    int is_super_chip = IS_SUPER_CHIP;
    unsigned char memory[MEMORY_SIZE];
    unsigned short program_counter = PROGRAM_STARTING_ADDRESS;
    unsigned short index_register = 0x0000;
    unsigned short stack[STACK_SIZE];
    unsigned char stack_top_pointer = 0x00;
    unsigned char delay_timer = 0xFF;
    unsigned char sound_timer = 0xFF;
    unsigned char general_purpose_registers[NUMBER_GENERAL_PURPOSE_REGISTERS];

    init_memory(memory, stack, general_purpose_registers);
    load_rom(memory, argv[1]);

    // Init SDL screen stuff.
    SDL_Texture * sdl_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
    unsigned int pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    clean_screen_pixels(pixels);

    unsigned char pressed_keys[17];
    for(int i=0; i<17; i++)
        pressed_keys[i] = 0x00;

    while(!quit){
        get_keyboard_input(event, pressed_keys);
        if(pressed_keys[0x10] == 0x01)
            quit = 1;
        update_timers(&delay_timer, &sound_timer);
        unsigned short instruction = fetch_instruction(memory, &program_counter);
        decode_run(instruction, &program_counter, stack, &stack_top_pointer, general_purpose_registers, &index_register,
                   pixels, memory, pressed_keys, &delay_timer, &sound_timer, is_super_chip);
        
        if((instruction & 0xF000) == 0xD000){
            SDL_UpdateTexture(sdl_texture, NULL, pixels, SCREEN_WIDTH * sizeof(int));    
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, sdl_texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
        SDL_Delay(DELAY);
    } 

    SDL_DestroyTexture(sdl_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}