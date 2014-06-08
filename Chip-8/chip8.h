/*
 * chip8.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifndef __CHIP8__ 
#define __CHIP8__

#define OPCODE_LEN 2

class Chip8 {
    
    public:
    
        bool LoadApplication(const char * filename);
        void EmulateCycle();
        void SetKeys();
        
        // Graphics
        unsigned char gfx[64 * 32]; // 2048 total pixels
        bool drawFlag;              // Frame ready to draw
       
        // I/O
        unsigned char key[16];      // Hex based keypad input
        
    private:
    
        void Initialize();          // Initialize emulation state
    
        // Timers
        unsigned char timer_delay;  // Delay timer
        unsigned char timer_sound;  // Sound timer
        
        // Memory
        unsigned char memory[4096]; // Memory (size = 4k)
        unsigned short stack[16];   // Stack (16 levels)
        
        // Registers
        unsigned char V[16];        // 16 8-bit data registers (V0 - VF)
        unsigned short I;           // Address Register
        unsigned short pc;          // Program Counter  (0x000 to 0xFFF)
        unsigned short sp;          // Stack Pointer        
        unsigned short opcode;      // Working area for active opcode
};

#endif
