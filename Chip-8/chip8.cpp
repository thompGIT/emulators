/*
 * chip8.cpp 
 *  - References: 
 *      http://www.multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
 */

#include "chip8.h"

unsigned char chip8_fontset[80] =
{ 
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

void Chip8::Initialize() {
    
    pc  = 0x200;        // PC starts at 0x200
    sp  = 0x00;         // Reset Stack Pointer
    I   = 0;            // Reset index register
    opcode  = 0;        // Reset current opcode
    
    // Clear display
    memset(gfx, 0x00, sizeof(gfx));
    drawFlag = true;
    
    // Clear stack
    memset(stack, 0x00, sizeof(stack));   
    
    // Clear registers
    memset(V, 0x00, sizeof(V));
    
    // Clear memory
    memset(memory, 0x00, sizeof(memory)); 
        
    // Clear IO
    memset(key, 0x00, sizeof(key));
        
    // Load fontset
    for(unsigned int i=0; i<80; ++i) {
        memory[i] = chip8_fontset[i];
    }

    // Reset timers
    timer_delay = 0;
    timer_sound = 0;
    
    // Seed rand
	srand (time(NULL));
}

bool Chip8::LoadApplication(const char * filename)
{
    Initialize();
    printf("Loading: %s\n", filename);
        
    // Open file
    FILE * pFile = fopen(filename, "rb");
    if (pFile == NULL) {
        fputs ("File error", stderr); 
        return false;
    }

    // Check file size
    fseek(pFile , 0 , SEEK_END);
    long lSize = ftell(pFile);
    rewind(pFile);
    printf("Filesize: %d\n", (int)lSize);
	
	// Allocate memory to contain the whole file
	char * buffer = (char*)malloc(sizeof(char) * lSize);
	if (buffer == NULL) {
        fputs ("Memory error", stderr); 
        return false;
    }

    // Copy the file into the buffer
    size_t result = fread (buffer, 1, lSize, pFile);
    if (result != (size_t)lSize) {
        fputs("Reading error",stderr); 
        return false;
    }

    // Copy buffer to Chip8 memory
    if((4096-512) > lSize) {
        for(int i = 0; i < lSize; ++i) {
            memory[i + 512] = buffer[i];
        }
    } else {
        printf("Error: ROM too big for memory");
    }
	
    // Close file, free buffer
    fclose(pFile);
    free(buffer);

    return true;
}

void Chip8::EmulateCycle() {
    
    // Fetch Opcode
    opcode = (memory[pc] << 8) | (memory[pc + 1]);
    
    // Decode / Execute Opcode
    switch (opcode & 0xF000) {
        case 0x0000: {
            switch(opcode & 0x000F) {   
                case 0x0000: { // 00E0       Clears the screen.                    
                    memset(gfx, 0x00, sizeof(gfx));
                    drawFlag = true;
                    pc += 2;
                    break;
                }
                case 0x000E: { // 00EE	    Returns from a subroutine.
                    sp--;
                    pc = stack[sp];
                    stack[sp] = 0x00;
                    pc += 2;
                    break;
                }
                default: {
                    printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
                    break;
                }
            }      
            break;
        }        
        case 0x1000: { // 1NNN	    Jumps to address NNN.
            pc = opcode & 0x0FFF;
            break;
        }
        case 0x2000: { // 2NNN	    Calls subroutine at NNN.
            stack[sp] = pc;
            sp++;
            pc = opcode & 0x0FFF;
            break;
        }
        case 0x3000: { // 3XNN	    Skips the next instruction if VX equals NN.
            if ( V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF) ) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0x4000: { // 4XNN	    Skips the next instruction if VX doesn't equal NN.
            if ( V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF) ) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0x5000: {  // 5XY0	    Skips the next instruction if VX equals VY.
            if ( V[(opcode & 0x0F00) >> 8] == V[opcode & 0x00F0 >> 4] ) {
                pc += 4;
            } else {
                pc += 2;
            }
            break;
        }
        case 0x6000: {  // 6XNN	    Sets VX to NN.
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;
        }
        case 0x7000: {  // 7XNN	    Adds NN to VX.
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;
        }
        case 0x8000: {
            switch (opcode & 0x000F) {
                case 0x0000: { // 8XY0	    Sets VX to the value of VY.
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                }
                case 0x0001: { // 8XY1	    Sets VX to VX or VY.
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                }
                case 0x0002: { // 8XY2	    Sets VX to VX and VY.
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                }
                case 0x0003: { // 8XY3	    Sets VX to VX xor VY.
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    pc += 2;
                    break;
                }
                case 0x0004: { // 8XY4	    Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
                    if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])) {
                        V[0xF] = 1;     // Carry
                    } else {
                        V[0xF] = 0;     // No Carry
                    }                    
                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];                    
                    pc += 2;                    
                    break;
                }
                case 0x0005: { // 8XY5	    VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
                    if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) {
                        V[0xF] = 0;     // No Carry
                    } else {
                        V[0xF] = 1;     // Carry
                    }                    
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];                    
                    pc += 2;   
                    break;
                }
                case 0x0006: { // 8XY6	    Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift.[2]
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x01;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    pc += 2;      
                    break;
                }
                case 0x0007: { // 8XY7	    Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
                    if (V[(opcode & 0x00F0) >> 4] < V[(opcode & 0x0F00) >> 8]) {
                        V[0xF] = 0;     // Borrow
                    } else {
                        V[0xF] = 1;     // No Borrow
                    }                    
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                    pc += 2;                       
                    break;
                }
                case 0x000E: { // 8XYE	    Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift.[2]
                    V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0x80) >> 7;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    pc += 2;      
                    break;
                }
                default: {
                    printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
                    break;
                }
            }    
            break;
        }
        case 0x9000: { // 9XY0	    Skips the next instruction if VX doesn't equal VY.
            if ( V[(opcode & 0x0F00) >> 8] != V[opcode & 0x00F0 >> 4] ) {
                pc += 4;
            }
            pc += 2;
            break;
        }
        case 0xA000: { // ANNN     Sets I to the address NNN.
            I = opcode & 0x0FFF;
            pc += 2;    	
            break;
        }
        case 0xB000: { // BNNN	    Jumps to the address NNN plus V0.
            pc = (opcode & 0x0FFF) + V[0];
            break;
        }
        case 0xC000: { // CXNN	    Sets VX to a random number and NN.
			V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
            break;
        }
        case 0xD000: { // DXYN	    Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. Each row of 8 pixels is read as bit-coded (with the most significant bit of each byte displayed on the left) starting from memory location I; I value doesn't change after the execution of this instruction. As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that doesn't happen.
            unsigned short x = V[(opcode & 0x0F00) >> 8];
            unsigned short y = V[(opcode & 0x00F0) >> 4];
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;
         
            V[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {
                pixel = memory[I + yline];
                for(int xline = 0; xline < 8; xline++) {
                    if((pixel & (0x80 >> xline)) != 0) {
                        if(gfx[(x + xline + ((y + yline) * 64))] == 1) {
                            V[0xF] = 1;
                        }
                        gfx[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }         
            drawFlag = true;
            pc += 2;
            break;
        }
        case 0xE000: { 
            switch (opcode & 0x00FF) {
                case 0x009E: { // EX9E	Skips the next instruction if the key stored in VX is pressed.                          
                    if(key[V[(opcode & 0x0F00) >> 8]] != 0) {
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                    break;
                }
                case 0x00A1: { // EXA1	Skips the next instruction if the key stored in VX isn't pressed.               
                    if(key[V[(opcode & 0x0F00) >> 8]] == 0) {
                        pc += 4;
                    } else {
                        pc += 2;
                    }
                    break;
                }
                default: {
                    printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
                    break;
                }
            }    
            break;
        }
        case 0xF000: {
            switch (opcode & 0x00FF) {
                case 0x0007: { // FX07	Sets VX to the value of the delay timer.
                    V[(opcode & 0x0F00) >> 8] = timer_delay;
                    pc += 2;
                    break;
                }
                case 0x000A: { // FX0A	A key press is awaited, and then stored in VX.
                    					
                    bool keyPress = false;

                    for(int i = 0; i < 16; ++i) {
                        if(key[i] != 0) {
                            V[(opcode & 0x0F00) >> 8] = i;
                            keyPress = true;
                        }
                    }

                    // If we didn't received a keypress, skip this cycle and try again.
                    if(!keyPress)						
                        return;
                        
                    break;
                }
                case 0x0015: { // FX15	Sets the delay timer to VX.
                    timer_delay = V[(opcode & 0x0F00) >> 8];
                    pc += 2;    	
                    break;
                }
                case 0x0018: { // FX18	Sets the sound timer to VX.
                    timer_sound = V[(opcode & 0x0F00) >> 8];
                    pc += 2;    
                    break;
                }
                case 0x001E: { // FX1E	Adds VX to I.
                    if (I + V[(opcode & 0x0F00) >> 8] > 0x0FFF) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                    I += V[(opcode & 0x0F00) >> 8];     
                    pc += 2;               
                    break;
                }
                case 0x0029: { // FX29	Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
                    I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    pc += 2;
                    break;
                }
                case 0x0033: { // FX33	Stores the Binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2. (In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.)
                    memory[I]     =  V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
                    pc += 2;
                    break;
                }
                case 0x0055: { // FX55	Stores V0 to VX in memory starting at address I.
/*                    
                    memcpy(memory + I, V, ((opcode & 0x0F00) >> 8) * 2);
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
*/                    

                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
                        memory[I + i] = V[i];	

                    // On the original interpreter, when the operation is done, I = I + X + 1.
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;

                    break;
                }
                case 0x0065: { // FX65	Fills V0 to VX with values from memory starting at address I.
/*                    memcpy((void*)V, memory + I, ((opcode & 0x0F00) >> 8) * 2);
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
*/
                    
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
                        V[i] = memory[I + i];			

                    // On the original interpreter, when the operation is done, I = I + X + 1.
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    pc += 2;
                   
                    break;
                }
                default: {
                    printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
                    break;
                }
            }        
            break;
        }
        default: {
            printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
            break;
        }
    }
        
    // Update Timers
    if (timer_delay > 0) {
        timer_delay--;
    }
    
    if (timer_sound > 0) {
        if(timer_sound == 1) {
            printf("BEEP!\n");
        }
        timer_sound--;
    }
}

void Chip8::SetKeys() {
}
