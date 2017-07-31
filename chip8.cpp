#include <iostream>
#include <iomanip>
#include <fstream>

typedef unsigned char   word_t; //  8-bit
typedef unsigned short dword_t; // 16-bit

#define MEMORY_SIZE 4096
#define PROGRAM_START_ADDR 512 // Most progs start at 0x200 (512)

class Interpreter
{
    private:
         word_t mem[MEMORY_SIZE];
         word_t registers_v[16];
        dword_t registers_i;
         word_t registers_sound_timer;
         word_t registers_delay_timer;
        dword_t stack[16];
         word_t stack_pointer;
        dword_t program_counter;

        // Bytes are expected to be interpreted as be
        void swap_endianness(dword_t& word)
        {
            word = word << 8 | word >> 8;
        }

      public:
        void load(const std::string rom_path)
        {
            std::ifstream stream(rom_path, std::ios::binary);
            if (stream.is_open())
            {
                stream.seekg(0, stream.end);
                const unsigned int len = stream.tellg();
                stream.seekg(0);
                stream.read(reinterpret_cast<char *>(&mem[PROGRAM_START_ADDR]), len);
            }
            else
            {
                std::cerr << "Unable to open rom file: " << rom_path << std::endl;
            }
            stream.close();
        }







        void debug() const
        {
            std::cout << "registers_v:" << std::endl;
            for (int i = 0; i < 16; i++)
            {
                std::cout << std::setw(4) << "V" << std::hex << i << ": " <<
                std::dec << registers_v[i] << std::endl;
            }
        }

        void mem_dump(char bytes)
        {
            for (int i = PROGRAM_START_ADDR; i < PROGRAM_START_ADDR + bytes; i += 2)
            {
                //endianness_swap(mem[i]);
                std::cout << std::hex << std::showbase << std::setw(4) << std::setfill('0') << i << ": "
                          << std::noshowbase << std::setw(2) << int(mem[i]) << std::setw(2) << int(mem[i+1])
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
    fish8.mem_dump(16);
}