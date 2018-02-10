#include <algorithm>
#include <bitset>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "interpreter.hpp"

#define PROG_START_ADDR 0x200 // Most programs start at 0x200 (512)

Interpreter::Interpreter()
{
    loadFontSprites();
    reset();
}

// Soft reset; does not clear memory
void Interpreter::reset()
{
    std::fill(std::begin(registersV),    std::end(registersV),    0);
    std::fill(std::begin(stack),         std::end(stack),         0);
    std::fill(std::begin(buffer.pixels), std::end(buffer.pixels), 0);
    std::fill(std::begin(keyState),      std::end(keyState),      false);

    registersI     = 0;
    registersST    = 0;
    registersDT    = 0;
    stackPointer   = 0;
    programCounter = PROG_START_ADDR;
}

bool Interpreter::loadProgram(const std::string& program)
{
    // Clear program memory
    std::fill(std::begin(mem)+PROG_START_ADDR, std::end(mem), 0);

    std::ifstream stream(program, std::ios::binary);
    if (stream.is_open())
    {
        stream.seekg(0, stream.end);
        const auto len = stream.tellg();

        // Can program fit in memory?
        if (len > MEMORY_SIZE - PROG_START_ADDR)
        {
            std::cerr << "Program too large! Max size is "
                      << (MEMORY_SIZE - PROG_START_ADDR) << " bytes"
                      << std::endl;
            return false;
        }

        stream.seekg(0);
        stream.read(reinterpret_cast<char*>(&mem[PROG_START_ADDR]), len);

        // Populate progInfo fields
        progInfo.size = len;
        const auto pos = program.find_last_of("/\\");
        if (pos != std::string::npos)
        {
            progInfo.path = program.substr(0, pos);
            progInfo.name = program.substr(pos+1);
        }
    }
    else
    {
        std::cerr << "Unable to open program '" << program << "'" << std::endl;
        return false;
    }
    stream.close();
    return true;
}

void Interpreter::cycle()
{
    const u16 inst = mem[programCounter] << 8 | mem[programCounter + 1];
    programCounter += 2;
    execute(inst);
}

// Timers should be decremented at 60 Hz
void Interpreter::cycleTimers()
{
    if (registersDT > 0) registersDT--;
    if (registersST > 0) registersST--;
}

void Interpreter::setKeyState(u8 hexKeyCode, bool isPressed)
{
    keyState[hexKeyCode & 0xF] = isPressed;
}

bool Interpreter::isBuzzerSet() const
{
    return registersST > 0;
}

const Interpreter::ProgramInfo& Interpreter::programInfo() const
{
    return progInfo;
}

const Interpreter::FrameBuffer& Interpreter::frameBuffer() const
{
    return buffer;
}

void Interpreter::loadFontSprites()
{
    // Each digit is represented by 5 bytes
    const u8 font[] = {
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

    // Copy into mem before PROG_START_ADDR, i.e. starting at 0x000
    std::copy(font, font+sizeof(font), mem);
}

void Interpreter::execute(u16 instruction)
{
    // Decode instruction
    const auto x   = (instruction & 0x0F00) >> 8;
    const auto y   = (instruction & 0x00F0) >> 4;
    const auto nnn =  instruction & 0x0FFF;
    const auto nn  =  instruction & 0x00FF;
    const auto n   =  instruction & 0x000F;

    // 00E0: Clear the screen
    if (instruction == 0x00E0)
    {
        std::fill(std::begin(buffer.pixels), std::end(buffer.pixels), 0);
    }
    // 00EE: Return from subroutine
    else if (instruction == 0x00EE)
    {
        programCounter = stack[--stackPointer];
    }
    // 1nnn: Jump to address nnn
    else if ((instruction & 0xF000) == 0x1000)
    {
        programCounter = nnn;
    }
    // 2nnn: Call subroutine at address nnn
    else if ((instruction & 0xF000) == 0x2000)
    {
        if (stackPointer < 16)
        {
            stack[stackPointer++] = programCounter;
            programCounter = nnn;
        }
    }
    // 3xnn: Skip next inst if Vx == nn
    else if ((instruction & 0xF000) == 0x3000)
    {
        if (registersV[x] == nn)
        {
            programCounter += 2;
        }
    }
    // 4xnn: Skip next inst if Vx != nn
    else if ((instruction & 0xF000) == 0x4000)
    {
        if (registersV[x] != nn)
        {
            programCounter += 2;
        }
    }
    // 5xy0: Skip next inst if Vx == Vy
    else if ((instruction & 0xF00F) == 0x5000)
    {
        if (registersV[x] == registersV[y])
        {
            programCounter += 2;
        }
    }
    // 6xnn: Set Vx = nn
    else if ((instruction & 0xF000) == 0x6000)
    {
        registersV[x] = nn;
    }
    // 7xnn: Set Vx = Vx + nn
    else if ((instruction & 0xF000) == 0x7000)
    {
        registersV[x] += nn;
    }
    // 8xy0: Set Vx = Vy
    else if ((instruction & 0xF00F) == 0x8000)
    {
        registersV[x] = registersV[y];
    }
    // 8xy1: Set Vx = Vx OR Vy
    else if ((instruction & 0xF00F) == 0x8001)
    {
        registersV[x] |= registersV[y];
    }
    // 8xy2: Set Vx = Vx AND Vy
    else if ((instruction & 0xF00F) == 0x8002)
    {
        registersV[x] &= registersV[y];
    }
    // 8xy3: Set Vx = Vx XOR Vy
    else if ((instruction & 0xF00F) == 0x8003)
    {
        registersV[x] ^= registersV[y];
    }
    // 8xy4: Set Vx = Vx + Vy. Vf = 1 if carry occurs
    else if ((instruction & 0xF00F) == 0x8004)
    {
        registersV[x]  += registersV[y];
        registersV[0xF] = registersV[x] > 0xFF ? 1 : 0;
    }
    // 8xy5: Set Vx = Vx - Vy. Vf = 1 if borrow occurs
    else if ((instruction & 0xF00F) == 0x8005)
    {
        registersV[0xF] = registersV[x] >= registersV[y] ? 1 : 0;
        registersV[x]  -= registersV[y];
    }
    // 8xy6: Set Vx = Vx shr 1 *or* Vx = Vy shr 1 depending on doc
    else if ((instruction & 0xF00F) == 0x8006)
    {
        // VF set to least sig bit before shift
        registersV[0xF] = registersV[x] & 0x1;
        registersV[x]   = registersV[y] >> 1;
        // registersV[x] >>= 1;
    }
    // 8xy7: Set Vx = Vy - Vx. Vf = 1 if borrow occurs
    else if ((instruction & 0xF00F) == 0x8007)
    {
        registersV[0xF] = registersV[y] >= registersV[x] ? 1 : 0;
        registersV[x]   = registersV[y]  - registersV[x];
    }
    // 8xyE: Set Vx = Vx shl 1 *or* Vx = Vy shl 1 depending on doc
    else if ((instruction & 0xF00F) == 0x800E)
    {
        // Vf set to most sig bit before shift
        registersV[0xF] = registersV[x] >> 7;
        registersV[x]   = registersV[y] << 1;
        // registersV[x] <<= 1;
    }
    // 9xy0: Skip next inst if Vx != Vy
    else if ((instruction & 0xF00F) == 0x9000)
    {
        if (registersV[x] != registersV[y])
        {
            programCounter += 2;
        }
    }
    // Annn: Set register I = address nnn
    else if ((instruction & 0xF000) == 0xA000)
    {
        registersI = nnn;
    }
    // Bnnn: Jump to address nnn + V0
    else if ((instruction & 0xF000) == 0xB000)
    {
        programCounter = nnn + registersV[0];
    }
    // Cxnn: Set Vx = random (0-255) AND nn
    else if ((instruction & 0xF000) == 0xC000)
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        registersV[x] = (std::rand() % 256) & nn;
    }
    // Dxyn: Draw n bytes at position Vx, Vy.
    else if ((instruction & 0xF000) == 0xD000)
    {
        drawToBuffer(x, y, n);
    }
    // Ex9E: Skip next inst if key == Vx is pressed
    else if ((instruction & 0xF0FF) == 0xE09E)
    {
        if (keyState[registersV[x] & 0xF])
        {
            programCounter += 2;
        }
    }
    // ExA1: Skip next inst if key == Vx is not pressed
    else if ((instruction & 0xF0FF) == 0xE0A1)
    {
        if (!keyState[registersV[x] & 0xF])
        {
            programCounter += 2;
        }
    }
    // Fx07: Set Vx = DT
    else if ((instruction & 0xF0FF) == 0xF007)
    {
        registersV[x] = registersDT;
    }
    // Fx0A: Wait for key press and set Vx = result
    else if ((instruction & 0xF0FF) == 0xF00A)
    {
        for (auto i = 0; i < 16; i++)
        {
            if (keyState[i])
            {
                registersV[x] = i;
                return;
            }
        }
        // If no key was pressed, repeat this inst
        programCounter -= 2;
    }
    // Fx15: Set DT = Vx
    else if ((instruction & 0xF0FF) == 0xF015)
    {
        registersDT = registersV[x];
    }
    // Fx18: Set ST = Vx
    else if ((instruction & 0xF0FF) == 0xF018)
    {
        registersST = registersV[x];
    }
    // Fx1E: Set register I = I + Vx
    else if ((instruction & 0xF0FF) == 0xF01E)
    {
        registersI += registersV[x];
    }
    // Fx29: Set register I = address of sprite data corresponding to Vx
    else if ((instruction & 0xF0FF) == 0xF029)
    {
        registersI = registersV[x] * 5;
    }
    // Fx33: Set register I, I+1, I+2 = binary-coded decimal of Vx
    else if ((instruction & 0xF0FF) == 0xF033)
    {
        mem[registersI]   = registersV[x] / 100;
        mem[registersI+1] = registersV[x] % 100 / 10;
        mem[registersI+2] = registersV[x] % 100 % 10;
    }
    // Fx55: Store V0..Vx in mem starting at address in register I
    else if ((instruction & 0xF0FF) == 0xF055)
    {
        for (auto i = 0; i <= x; i++)
        {
            mem[registersI+i] = registersV[i];
        }
        registersI += x + 1;
    }
    // Fx65: Fill V0..Vx from mem starting at address in register I
    else if ((instruction & 0xF0FF) == 0xF065)
    {
        for (auto i = 0; i <= x; i++)
        {
            registersV[i] = mem[registersI+i];
        }
        registersI += x + 1;
    }
    else
    {
        printf("Unrecognised instruction '%04x' @ 0x%04x\n",
            instruction, programCounter - 2);

        dumpRegisters();
        dumpMemory(32, programCounter - 2);
        exit(1);
    }
}

void Interpreter::drawToBuffer(u8 x, u8 y, u8 n)
{
    registersV[0xF] = 0;
    for (auto row = 0; row < n; row++)
    {
        const auto rowByte = mem[registersI + row];
        for (auto col = 0; col < 8; col++)
        {
            // Process bits left-to-right
            const auto spritePx = rowByte & (0x80 >> col);
            if (spritePx == 0)
            {
                // Ignore 'off' px; a px is turned 'off' by re-drawing
                continue;
            }

            const auto pxX = (registersV[x] & 0x3F) + col;
            const auto pxY = (registersV[y] & 0x1F) + row;
            auto bufferPx = buffer.pixels + pxX + pxY * buffer.Width;
            if (*bufferPx == 1)
            {
                // Collision handling:
                // Vf should be set to 1 if any 'on' px are changed to 'off'
                registersV[0xF] = 1;
            }

            // Px are drawn by XOR-ing
            *bufferPx ^= 1;
        }
    }
}

void Interpreter::dumpRegisters() const
{
    printf("registersV\n");
    for (auto i = 0; i < 16; i++)
    {
        printf("  V%x: %X\n", i, registersV[i]);
    }
    printf("registersI : %X\n", registersI);
    printf("registersDT: %X\n", registersDT);
    printf("registersST: %X\n", registersST);
}

void Interpreter::dumpMemory(u8 bytes, u8 offset) const
{
    printf("memory snapshot (%d bytes)\n", bytes);
    for (auto i = offset; i < offset + bytes; i += 2)
    {
        printf("  0x%04x: %02X %02X\n", i, mem[i], mem[i+1]);
    }
}
