#ifndef CHIP8_INTERPRETER_H_
#define CHIP8_INTERPRETER_H_

#include <string>

#define MEMORY_SIZE 4096
#define PROGRAM_START_ADDR 512 // Most progs start at 0x200 (512)

enum class OpCode
{
    ADD_Vx_NN,
    BCD_Vx,
    CALL_NNN,
    DRW_Vx_Vy_N,
    LD_DT_Vx,
    LD_Vx_DT,
    LD_I_NNN,
    LD_Vx_NN,
    LD_Vx_I,
    LD_I_Vx,
    LD_F_Vx,
    SE_Vx_NN,
    JMP_NNN,
    RND_Vx_NN,
    SKNP_Vx,
    RET,
    NOOP
};

class Interpreter
{
    public:
        Interpreter();

        // Loads ROM into interpreter's memory
        void load(std::string rom_path);

        // Do stuff
        void run();

        // Debugging
        void __reg_dump() const;
        void __mem_dump(int bytes, int offset) const;

    private:
        typedef unsigned char   Word; //  8-bit TODO: Word size might be 16-bit -- check.
        typedef unsigned short DWord; // 16-bit

         Word mem[MEMORY_SIZE] {/* value init */};
         Word registers_v[16] {/* value init */};
        DWord registers_i = 0;
         Word registers_sound_timer = 0;
         Word registers_delay_timer = 0;
        DWord stack[16] {/* value init */};
         Word stack_pointer = 0;
        DWord program_counter = PROGRAM_START_ADDR;

        // Swap word from big to little endian (or vice versa).
        // Bytes are expected to be interpreted as big endian
        void swap_endian(DWord &word);

        // Decode instruction opcode
        OpCode opcode(DWord instruction);

        // Execute instruction
        void execute_instruction(DWord instruction);

        // Create default hexidecimal font sprites. Sprites are loaded into mem between 0x000 and 0x199
        void create_font_sprites();
};

#endif // CHIP8_INTERPRETER_H_