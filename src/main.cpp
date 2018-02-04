#include <iostream>
#include "chip8.hpp"

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " path\\to\\rom" << std::endl;
        return 1;
    }

    Interpreter vm;
    vm.loadRom(argv[1]);

    Chip8 app(&vm);
    app.run();
}
