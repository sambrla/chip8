#include <iostream>
#include "interpreter.hpp"

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return 1;
    }

    Interpreter chip8;
    chip8.load_rom(argv[1]);
    chip8.run();
}