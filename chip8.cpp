#include <iostream>
#include <iomanip>
#include <fstream>
#include <bitset>
#include <cstring>

typedef unsigned char  byte_t; //  8-bit
typedef unsigned short word_t; // 16-bit

#define MEMORY_SIZE 4096
#define PROGRAM_START_ADDR 512 // Most progs start at 0x200 (512)

enum class OpCode
{
    BCD_Vx,
    CALL,
    DRW,
    LD_I,
    LD_Vx_byte,
    LD_Vx_I,
    LD_I_Vx,
    LD_F_Vx,
    RET,
    NOOP
};

class Interpreter
{
    private:
        byte_t mem[MEMORY_SIZE] {/* value init */};
        byte_t registers_v[16] {/* value init */};
        word_t registers_i = 0;
        byte_t registers_sound_timer = 0;
        byte_t registers_delay_timer = 0;
        word_t stack[16] {/* value init */};
        byte_t stack_pointer = 0;
        word_t program_counter = PROGRAM_START_ADDR;

        // Bytes are expected to be interpreted as be
        void swap_endianness(word_t &word)
        {
            word = word << 8 | word >> 8;
        }

        // Get opcode from instruction
        OpCode opcode(word_t instruction)
        {
            if ((instruction  & 0xF000) == 0xA000) return OpCode::LD_I;
            if ((instruction  & 0xF000) == 0x6000) return OpCode::LD_Vx_byte;
            if ((instruction  & 0xF0FF) == 0xF055) return OpCode::LD_I_Vx;
            if ((instruction  & 0xF0FF) == 0xF065) return OpCode::LD_Vx_I;
            if ((instruction  & 0xF0FF) == 0xF029) return OpCode::LD_F_Vx;
            if ((instruction  & 0xF000) == 0xD000) return OpCode::DRW;
            if ((instruction  & 0xF000) == 0x2000) return OpCode::CALL;
            if ((instruction  & 0xF0FF) == 0xF033) return OpCode::BCD_Vx;
            if  (instruction == 0x00EE)            return OpCode::RET;
            return OpCode::NOOP;
        }

        void execute_instruction(word_t instruction)
        {
            switch (opcode(instruction))
            {
                // 6xkk -> Load Vx with kk
                case OpCode::LD_Vx_byte:
                {
                    byte_t reg = instruction >> 8 & 0x0F;
                    byte_t val = instruction;
                    registers_v[int(reg)] = val;
                    break;
                }
                // Annn -> Store nnn in reg I
                case OpCode::LD_I:
                {
                    registers_i = instruction & 0x0FFF;
                    break;
                }
                // Dxyn -> Draw n-byte sprite at Vx,Vy
                case OpCode::DRW:
                {
                    const byte_t n = instruction & 0x000F;
                    byte_t* sprite = new byte_t[n];

                    // Copy sprite data from mem starting at addr reg I. Sprite size is 8xN
                    memcpy(sprite, &mem[registers_i], n);

                    // TODO: Draw sprite

                    delete sprite;
                    break;
                }
                // 2nnn -> Call subroutine at nnn
                case OpCode::CALL:
                {
                    stack_pointer++; // TODO: Stack[0] might need to be initially set to PROGRAM_START_ADDR
                    stack[stack_pointer] = program_counter;
                    program_counter = instruction & 0x0FFF;
                    return; // Don't want the pc to increment
                }
                // Return
                case OpCode::RET:
                {
                    program_counter = stack[stack_pointer];
                    stack_pointer--;
                    break;
                }

                // Fx33 -> Store binary-coded decimal of Vx in mem at addresses reg I (100), I+1 (10), I+2 (1)
                case OpCode::BCD_Vx:
                {
                    auto  x = instruction >> 8 & 0x0F;
                    auto vx = registers_v[x];

                    mem[registers_i]   = vx/100; vx %= 100; // 100
                    mem[registers_i+1] = vx/10;             // 10
                    mem[registers_i+2] = vx%10;             // 1
                    break;
                }

                // Fx55 -> Load mem at reg I..n with registers V0..Vx (inc.)
                case OpCode::LD_I_Vx:
                {
                    auto x = instruction >> 8 & 0x0F;
                    for (int i = 0; i <= x; i++, registers_i++)
                    {
                        mem[registers_i] = registers_v[i];
                    }
                    break;
                }

                // Fx65 -> Load registers V0..Vx (inc.) with values stored in mem at reg I..n
                case OpCode::LD_Vx_I:
                {
                    auto x = instruction >> 8 & 0x0F;
                    for (int i = 0; i <= x; i++, registers_i++)
                    {
                        registers_v[i] = mem[registers_i];
                    }
                    break;
                }

                // Fx29 -> Set reg I to mem addr of font sprite referred to in Vx
                case OpCode::LD_F_Vx:
                {

                    break;
                }

                default:
                {
                    std::cerr << "Unrecognised instruction @ " << std::dec << program_counter << ": "
                              << std::hex << instruction << std::endl;
                    reg_dump();
                    exit(1);
                }
            }

            program_counter += 2;
        }

        void create_font_sprites()
        {
            // 0
            mem[0] = 0xF0;
            mem[1] = 0x90;
            mem[2] = 0x90;
            mem[3] = 0x90;
            mem[4] = 0xF0;

            // 1
            mem[5] = 0x20;
            mem[6] = 0x60;
            mem[7] = 0x20;
            mem[8] = 0x20;
            mem[9] = 0x70;

            // 2
            mem[10] = 0xF0;
            mem[11] = 0x10;
            mem[12] = 0xF0;
            mem[13] = 0x80;
            mem[14] = 0xF0;

            // 3
            mem[15] = 0xF0;
            mem[16] = 0x10;
            mem[17] = 0xF0;
            mem[18] = 0x10;
            mem[19] = 0xF0;

            // 4
            mem[20] = 0x90;
            mem[21] = 0x90;
            mem[22] = 0xF0;
            mem[23] = 0x10;
            mem[24] = 0x10;

            // 5
            mem[25] = 0xF0;
            mem[26] = 0x80;
            mem[27] = 0xF0;
            mem[28] = 0x10;
            mem[29] = 0xF0;

            // 6
            mem[30] = 0xF0;
            mem[31] = 0x80;
            mem[32] = 0xF0;
            mem[33] = 0x90;
            mem[34] = 0xF0;

            // 7
            mem[35] = 0xF0;
            mem[36] = 0x10;
            mem[37] = 0x20;
            mem[38] = 0x40;
            mem[39] = 0x40;

            // 8
            mem[40] = 0xF0;
            mem[41] = 0x90;
            mem[42] = 0xF0;
            mem[43] = 0x90;
            mem[44] = 0xF0;

            // 9
            mem[45] = 0xF0;
            mem[46] = 0x90;
            mem[47] = 0xF0;
            mem[48] = 0x10;
            mem[49] = 0xF0;

            // A
            mem[50] = 0xF0;
            mem[51] = 0x90;
            mem[52] = 0xF0;
            mem[53] = 0x90;
            mem[54] = 0x90;

            // B
            mem[55] = 0xE0;
            mem[56] = 0x90;
            mem[57] = 0xE0;
            mem[58] = 0x90;
            mem[59] = 0xE0;

            // C
            mem[60] = 0xF0;
            mem[61] = 0x80;
            mem[62] = 0x80;
            mem[63] = 0x80;
            mem[64] = 0xF0;

            // D
            mem[65] = 0xE0;
            mem[66] = 0x90;
            mem[67] = 0x90;
            mem[68] = 0x90;
            mem[69] = 0xE0;

            // E
            mem[70] = 0xF0;
            mem[71] = 0x80;
            mem[72] = 0xF0;
            mem[73] = 0x80;
            mem[74] = 0xF0;

            // F
            mem[75] = 0xF0;
            mem[76] = 0x80;
            mem[77] = 0xF0;
            mem[78] = 0x80;
            mem[79] = 0x80;
        }

    public:
        Interpreter()
        {
            create_font_sprites();
        }

        // Load CHIP-8 rom into interpreter
        void load(std::string rom_path)
        {
            std::ifstream stream(rom_path, std::ios::binary);
            if (stream.is_open())
            {
                stream.seekg(0, stream.end);
                const auto len = stream.tellg();
                stream.seekg(0);
                stream.read(reinterpret_cast<char *>(&mem[PROGRAM_START_ADDR]), len);
            }
            else
            {
                std::cerr << "Unable to open rom file: " << rom_path << std::endl;
            }
            stream.close();
        }

        void run()
        {
            while (true)
            {
                const word_t instruction = mem[program_counter] << 8 | mem[program_counter+1];

                std::cout << "Executing instruction @ " << std::dec << program_counter << ": "
                          << std::hex << instruction << std::endl;

                execute_instruction(instruction);
            }
        }






        void reg_dump() const
        {
            std::cout << "program_counter: " << int(program_counter) << std::endl;

            std::cout << "registers_v:" << std::endl;
            for (int i = 0; i < 16; i++)
            {
                std::cout << std::setw(4) << "V" << std::hex << i << ": "
                          << std::dec << int(registers_v[i])
                          << std::endl;
            }
            std::cout << "registers_i: " << int(registers_i) << std::endl;

            std::cout << "stack:" << std::endl;
            for (int i = 0; i < 16; i++)
            {
                std::cout << std::setw(4) << "S" << std::hex << i << ": "
                          << std::dec << int(stack[i])
                          << std::endl;
            }
            std::cout << "stack_pointer: " << int(stack_pointer) << std::endl;
        }

        void mem_dump(uint16_t bytes) const
        {
            for (int i = PROGRAM_START_ADDR; i < PROGRAM_START_ADDR + bytes; i += 2)
            {
                //endianness_swap(mem[i]);
                std::cout << std::hex << std::showbase << std::setw(4) << std::setfill('0') << i << ": "
                          << std::noshowbase << std::setw(2) << int(mem[i]) << " " << std::setw(2) << int(mem[i+1])
                          << std::endl;
            }
        }
};

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return 1;
    }

    Interpreter fish8;
    fish8.load(argv[1]);
    fish8.run();
    //fish8.mem_dump(512);
}