#include <iostream>
#include <iomanip>
#include <fstream>
#include <bitset>
#include <cstring>
#include <ctime>

typedef unsigned char  byte_t; //  8-bit
typedef unsigned short word_t; // 16-bit

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
            if ((instruction  & 0xF000) == 0x7000) return OpCode::ADD_Vx_NN;
            if ((instruction  & 0xF0FF) == 0xF015) return OpCode::LD_DT_Vx;
            if ((instruction  & 0xF0FF) == 0xF007) return OpCode::LD_Vx_DT;
            if ((instruction  & 0xF000) == 0xA000) return OpCode::LD_I_NNN;
            if ((instruction  & 0xF000) == 0x6000) return OpCode::LD_Vx_NN;
            if ((instruction  & 0xF0FF) == 0xF055) return OpCode::LD_I_Vx;
            if ((instruction  & 0xF0FF) == 0xF065) return OpCode::LD_Vx_I;
            if ((instruction  & 0xF0FF) == 0xF029) return OpCode::LD_F_Vx;
            if ((instruction  & 0xF000) == 0xD000) return OpCode::DRW_Vx_Vy_N;
            if ((instruction  & 0xF000) == 0x2000) return OpCode::CALL_NNN;
            if ((instruction  & 0xF0FF) == 0xF033) return OpCode::BCD_Vx;
            if ((instruction  & 0xF000) == 0x3000) return OpCode::SE_Vx_NN;
            if ((instruction  & 0xF000) == 0x1000) return OpCode::JMP_NNN;
            if ((instruction  & 0xF000) == 0xC000) return OpCode::RND_Vx_NN;
            if ((instruction  & 0xF0FF) == 0xE0A1) return OpCode::SKNP_Vx;
            if  (instruction == 0x00EE)            return OpCode::RET;
            return OpCode::NOOP;
        }

        void execute_instruction(word_t instruction)
        {
            switch (opcode(instruction))
            {
                // 6xnn -> Load Vx with nn
                case OpCode::LD_Vx_NN:
                {
                    auto x = instruction >> 8 & 0x0F;
                    registers_v[x] = instruction & 0x00FF;
                    break;
                }
                // Annn -> Store nnn in reg I
                case OpCode::LD_I_NNN:
                {
                    registers_i = instruction & 0x0FFF;
                    break;
                }
                // Dxyn -> Draw n-byte sprite at Vx,Vy
                case OpCode::DRW_Vx_Vy_N:
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
                case OpCode::CALL_NNN:
                {
                    stack_pointer++;
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

                    mem[registers_i]   = vx / 100; vx %= 100; // 100
                    mem[registers_i+1] = vx / 10;             // 10
                    mem[registers_i+2] = vx % 10;             // 1
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
                    auto x = instruction >> 8 & 0x0F;
                    registers_i = mem[x*5]; // Each sprite is 5b and stored in mem contiguously starting at 0x000
                    break;
                }

                // 7xnn -> Vx = Vx + nn
                case OpCode::ADD_Vx_NN:
                {
                    auto x = instruction >> 8 & 0x0F;
                    registers_v[x] += instruction & 0x00FF;
                    break;
                }

                // Fx15 -> Load DT with Vx
                case OpCode::LD_DT_Vx:
                {
                    auto x = instruction >> 8 & 0x0F;
                    registers_delay_timer = registers_v[x];
                    break;
                }

                // Fx07 -> Load Vx with DT
                case OpCode::LD_Vx_DT:
                {
                    auto x = instruction >> 8 & 0x0F;
                    registers_v[x] = registers_delay_timer;
                    break;
                }

                // 3xnn -> Skip next instruction if Vx == nn
                case OpCode::SE_Vx_NN:
                {
                    auto  x = instruction >> 8 & 0x0F;
                    auto nn = instruction & 0x00FF;

                    if (registers_v[x] == nn)
                        program_counter += 2;

                    break;
                }

                // 1nnn -> Jump to addr nnn
                case OpCode::JMP_NNN:
                {
                    program_counter = instruction & 0x0FFF;
                    return; // Don't want the pc to increment
                }

                // Cxnn -> Generate random number between 0-255. AND with nn and store in Vx
                case OpCode::RND_Vx_NN:
                {
                    auto  x = instruction >> 8 & 0x0F;
                    auto nn = instruction & 0x00FF;

                    std::srand(std::time(0));
                    auto rand = std::rand() % 256;

                    registers_v[x] = rand & nn;
                    break;
                }

                // ExA1 -> Skip next instruction if key(Vx) not pressed
                case OpCode::SKNP_Vx:
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
            const byte_t font[] = {
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
            memcpy(mem, font, sizeof(font));
        }

    public:
        Interpreter()
        {
            stack[stack_pointer] = program_counter;
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

                // TODO: Supposed to count down at a rate of 60Hz.
                if (registers_delay_timer > 0) registers_delay_timer--;

                // TODO: Also counts down at a rate of 60Hz; however, if > 0, plays a sound
                if (registers_sound_timer > 0) registers_sound_timer--;
            }
        }






        void reg_dump() const
        {
            std::cout << "program_counter: " << std::dec << int(program_counter) << std::endl;
            std::cout << "registers_delay: " << std::dec << int(registers_delay_timer) << std::endl;
            std::cout << "registers_sound: " << std::dec << int(registers_sound_timer) << std::endl;

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

        void mem_dump(int bytes, int offset) const
        {
            for (int i = 0 + offset; i < offset + bytes; i += 2)
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
    //fish8.mem_dump(128, 0);
}