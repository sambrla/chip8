#ifndef CHIP8_INTERPRETER_H_
#define CHIP8_INTERPRETER_H_

#include <string>
#include "common.hpp"

#define MEMORY_SIZE 4096
#define PROGRAM_START_ADDR 512 // Most progs start at 0x200 (512)

enum class OpCode
{
    ADD_I_Vx,
    ADD_Vx_NN,
    ADD_Vx_Vy,
    AND_Vx_Vy,
    BCD_Vx,
    CALL_NNN,
    CLS,
    DRW_Vx_Vy_N,
    JMP_NNN,
    JMP_V0_NNN,
    LD_DT_Vx,
    LD_ST_Vx,
    LD_B_Vx,
    LD_F_Vx,
    LD_I_NNN,
    LD_I_Vx,
    LD_Vx_DT,
    LD_Vx_I,
    LD_Vx_K,
    LD_Vx_NN,
    LD_Vx_Vy,
    NOOP,
    OR_Vx_Vy,
    RET,
    RND_Vx_NN,
    SE_Vx_NN,
    SE_Vx_Vy,
    SHL_Vx,
    SHR_Vx,
    SKP_Vx,
    SKNP_Vx,
    SNE_Vx_NN,
    SNE_Vx_Vy,
    SUB_Vx_Vy,
    SUBN_Vx_Vy,
    XOR_Vx_Vy
};

class Interpreter
{
public: 
    Interpreter();

    void cycle();
    void load_rom(std::string rom_path);

    // Get frame buffer pixles. Frame buffer size is 64x32 and is stored row-first
    const u8* get_frame_buffer() const;

    // Debugging
    void __reg_dump() const;
    void __mem_dump(int bytes, int offset) const;

private:
      u8 mem[MEMORY_SIZE] {/* value init */};
      u8 registers_v[16] {};
     u16 registers_i = 0;
      u8 registers_sound_timer = 0;
      u8 registers_delay_timer = 0;
     u16 stack[16] {};
      u8 stack_pointer = 0;
     u16 program_counter = PROGRAM_START_ADDR;
      u8 frame_buffer[64*32] {};
    bool key_state[16] {};
    
    void execute_instruction(u16 instruction);
    OpCode opcode(u16 instruction) const;
    
    // Create default hexidecimal font sprites. Sprites are loaded into mem between 0x000 and 0x199
    void load_font_sprites();
};

#endif // CHIP8_INTERPRETER_H_