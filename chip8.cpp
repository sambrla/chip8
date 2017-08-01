#include <iostream>
#include <iomanip>
#include <fstream>
#include <bitset>

//typedef unsigned char   uint8_t;
//typedef unsigned short uint16_t;

#define MEMORY_SIZE 4096
#define PROGRAM_START_ADDR 512 // Most progs start at 0x200 (512)

enum class OpCode
{
    LD_byte,
    NOOP
};

class Interpreter
{
    private:
         uint8_t mem[MEMORY_SIZE] {/* value init */};
         uint8_t registers_v[16] {/* value init */};
        uint16_t registers_i = 0;
         uint8_t registers_sound_timer = 0;
         uint8_t registers_delay_timer = 0;
        uint16_t stack[16] {/* value init */};
         uint8_t stack_pointer = 0;
        uint16_t program_counter = PROGRAM_START_ADDR;

        // Bytes are expected to be interpreted as be
        void swap_endianness(uint16_t &word)
        {
            word = word << 8 | word >> 8;
        }

        OpCode opcode(uint16_t instruction)
        {
            if ((instruction & 0x6000) == 0x6000) return OpCode::LD_byte;
            return OpCode::NOOP;
        }

        void execute_instruction(uint16_t instruction)
        {
            switch (opcode(instruction))
            {
                case OpCode::LD_byte: // 6xkk -> LD Vx with kk
                {
                    uint8_t reg = instruction >> 8 & 0xF;
                    uint8_t val = instruction;
                    registers_v[int(reg)] = val;
                    break;
                }
                default:
                {
                    std::cerr << "Unrecognised instruction: " << std::hex << instruction << std::endl;
                    reg_dump();
                    exit(1);
                }
            }
        }

    public:
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
                const uint16_t instruction = mem[program_counter] << 8 | mem[program_counter+1];
                execute_instruction(instruction);

                program_counter += 2;
            }
        }




        void reg_dump() const
        {
            std::cout << "registers_v:" << std::endl;
            for (int i = 0; i < 16; i++)
            {
                std::cout << std::setw(4) << "V" << std::hex << i << ": "
                          << std::dec << int(registers_v[i])
                          << std::endl;
            }
        }

        void mem_dump(uint8_t bytes) const
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
    //fish8.mem_dump(128);
}

// Note to self:
// C++14: clear && g++ -std=c++1y chip8.cpp -o fish-8
// C++11: clear && clang++ -std=c++11 chip8.cpp -o fish-8