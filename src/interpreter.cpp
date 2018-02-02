#include <bitset>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "interpreter.hpp"

Interpreter::Interpreter()
{
    createFontSprites();
}

void Interpreter::reset()
{
    // TODO:
}

void Interpreter::cycle()
{
    const u16 instruction = mem[programCounter] << 8 | mem[programCounter + 1]; // Fetch
    executeInstruction(instruction);
}

void Interpreter::cycleTimers()
{
    if (registersDelayTimer > 0) registersDelayTimer--;
    if (registersSoundTimer > 0) registersSoundTimer--;
}

void Interpreter::loadRom(std::string path)
{
    std::ifstream stream(path, std::ios::binary);
    if (stream.is_open())
    {
        stream.seekg(0, stream.end);
        const auto len = stream.tellg();

        if (len > MEMORY_SIZE - PROGRAM_START_ADDR)
        {
            std::cerr << "Unable to load rom. File too large!" << std::endl;
            return;
        }

        stream.seekg(0);
        stream.read(reinterpret_cast<char*>(&mem[PROGRAM_START_ADDR]), len);
        info.size = len;
    }
    else
    {
        std::cerr << "Unable to open rom file @: " << path << std::endl;
    }
    stream.close();

    // Extract path and file name
    auto pos = path.find_last_of("/\\");
    if (pos != std::string::npos)
    {
        info.path = path.substr(0, pos);
        info.name = path.substr(pos+1);
    }
}

const Interpreter::RomInfo* Interpreter::romInfo() const
{
    return &info;
}

void Interpreter::setKeyState(KeyCode key, bool isPressed)
{
    keyState[key] = isPressed;
}

bool Interpreter::shouldBeep() const
{
    return registersSoundTimer > 0;
}

const Interpreter::FrameBuffer* Interpreter::frame() const
{
    return &framebuffer;
}

// TODO: Efficiency! That's a lot of ifs to get to LD_Vx_I
Interpreter::OpCode Interpreter::opcode(u16 instruction) const // Decode
{
    if (instruction == 0x00E0)            return OpCode::CLS;
    if (instruction == 0x00EE)            return OpCode::RET;
    if ((instruction & 0xF000) == 0x1000) return OpCode::JP_NNN;
    if ((instruction & 0xF000) == 0x2000) return OpCode::CALL_NNN;
    if ((instruction & 0xF000) == 0x3000) return OpCode::SE_Vx_NN;
    if ((instruction & 0xF000) == 0x4000) return OpCode::SNE_Vx_NN;
    if ((instruction & 0xF00F) == 0x5000) return OpCode::SE_Vx_Vy;
    if ((instruction & 0xF000) == 0x6000) return OpCode::LD_Vx_NN;
    if ((instruction & 0xF000) == 0x7000) return OpCode::ADD_Vx_NN;
    if ((instruction & 0xF00F) == 0x8000) return OpCode::LD_Vx_Vy;
    if ((instruction & 0xF00F) == 0x8001) return OpCode::OR_Vx_Vy;
    if ((instruction & 0xF00F) == 0x8002) return OpCode::AND_Vx_Vy;
    if ((instruction & 0xF00F) == 0x8003) return OpCode::XOR_Vx_Vy;
    if ((instruction & 0xF00F) == 0x8004) return OpCode::ADD_Vx_Vy;
    if ((instruction & 0xF00F) == 0x8005) return OpCode::SUB_Vx_Vy;
    if ((instruction & 0xF00F) == 0x8006) return OpCode::SHR_Vx;
    if ((instruction & 0xF00F) == 0x8007) return OpCode::SUBN_Vx_Vy;
    if ((instruction & 0xF00F) == 0x800E) return OpCode::SHL_Vx;
    if ((instruction & 0xF00F) == 0x9000) return OpCode::SNE_Vx_Vy;
    if ((instruction & 0xF000) == 0xA000) return OpCode::LD_I_NNN;
    if ((instruction & 0xF000) == 0xB000) return OpCode::JP_V0_NNN;
    if ((instruction & 0xF000) == 0xC000) return OpCode::RND_Vx_NN;
    if ((instruction & 0xF000) == 0xD000) return OpCode::DRW_Vx_Vy_N;
    if ((instruction & 0xF0FF) == 0xE09E) return OpCode::SKP_Vx;
    if ((instruction & 0xF0FF) == 0xE0A1) return OpCode::SKNP_Vx;
    if ((instruction & 0xF0FF) == 0xF007) return OpCode::LD_Vx_DT;
    if ((instruction & 0xF0FF) == 0xF00A) return OpCode::LD_Vx_K;
    if ((instruction & 0xF0FF) == 0xF015) return OpCode::LD_DT_Vx;
    if ((instruction & 0xF0FF) == 0xF018) return OpCode::LD_ST_Vx;
    if ((instruction & 0xF0FF) == 0xF01E) return OpCode::ADD_I_Vx;
    if ((instruction & 0xF0FF) == 0xF029) return OpCode::LD_F_Vx;
    if ((instruction & 0xF0FF) == 0xF033) return OpCode::LD_B_Vx;
    if ((instruction & 0xF0FF) == 0xF055) return OpCode::LD_I_Vx;
    if ((instruction & 0xF0FF) == 0xF065) return OpCode::LD_Vx_I;

    // Although there is no NOOP opcode, we'll use it to signal an invalid instruction
    return OpCode::NOOP;
}

// TODO: Refactor
void Interpreter::executeInstruction(u16 instruction) // Execute
{
    const auto x   = (instruction & 0x0F00) >> 8;
    const auto y   = (instruction & 0x00F0) >> 4;
    const auto nnn =  instruction & 0x0FFF;
    const auto nn  =  instruction & 0x00FF;
    const auto n   =  instruction & 0x000F;

    //printf("0x%04x: %04x\n", programCounter, instruction);
    switch (opcode(instruction))
    {
        // Clear the display
        case OpCode::CLS:
        {
            const auto size = framebuffer.kWidth * framebuffer.kHeight;
            for (auto i = 0; i < size; i++)
            {
                framebuffer.pixels[i] = 0;
            }
            break;
        }

        // Return from subroutine
        case OpCode::RET:
        {
            programCounter = stack[--stackPointer];
            break;
        }

        // Jump to location nnn
        case OpCode::JP_NNN:
        {
            programCounter = nnn;
            return; // Don't want the pc to increment
        }

        // Call subroutine at nnn
        case OpCode::CALL_NNN:
        {
            stack[stackPointer++] = programCounter;
            programCounter = nnn;
            return; // Don't want the pc to increment
        }

        // Skip next instruction if Vx == nn
        case OpCode::SE_Vx_NN:
        {
            if (registersV[x] == nn)
            {
                programCounter += 2;
            }
            break;
        }

        // Skip next instruction if Vx != nn
        case OpCode::SNE_Vx_NN:
        {
            if (registersV[x] != nn)
            {
                programCounter += 2;
            }
            break;
        }

        // Skip next instruction if Vx == Vy
        case OpCode::SE_Vx_Vy:
        {
            if (registersV[x] == registersV[y])
            {
                programCounter += 2;
            }
            break;
        }

        // Set Vx = nn
        case OpCode::LD_Vx_NN:
        {
            registersV[x] = nn;
            break;
        }

        // Set Vx = Vx + nn
        case OpCode::ADD_Vx_NN:
        {
            registersV[x] += nn;
            break;
        }

        // Set Vx = Vy
        case OpCode::LD_Vx_Vy:
        {
            registersV[x] = registersV[y];
            break;
        }

        // Set Vx = Vx OR Vy
        case OpCode::OR_Vx_Vy:
        {
            registersV[x] |= registersV[y];
            break;
        }

        // Set Vx = Vx AND Vy
        case OpCode::AND_Vx_Vy:
        {
            registersV[x] &= registersV[y];
            break;
        }

        // Set Vx = Vx XOR Vy
        case OpCode::XOR_Vx_Vy:
        {
            registersV[x] ^= registersV[y];
            break;
        }

        // Set Vx = Vx + Vy, set VF = carry
        case OpCode::ADD_Vx_Vy:
        {
            auto sum = registersV[x] + registersV[y];
            registersV[0xF] = sum > 255 ? 1 : 0;
            registersV[x]   = static_cast<u8>(sum);
            break;
        }

        // Set Vx = Vx - Vy, set VF = NOT borrow
        case OpCode::SUB_Vx_Vy:
        {
            registersV[0xF] = registersV[x] > registersV[y] ? 1 : 0;
            registersV[x]  -= registersV[y];
            break;
        }

        // Set Vx = Vy SHR 1
        case OpCode::SHR_Vx:
        {
            // Least sig bit stored in VF prior to shift
            registersV[0xF] = instruction & 1;
            registersV[x] = registersV[y] >> 1;
            break;
        }

        // Set Vx = Vy - Vx, set VF = NOT borrow
        case OpCode::SUBN_Vx_Vy:
        {
            registersV[0xF] = registersV[y] > registersV[x] ? 1 : 0;
            registersV[x]   = registersV[y] - registersV[x];
            break;
        }

        // Set Vx = Vy SHL 1
        case OpCode::SHL_Vx:
        {
            // Most sig bit stored in VF prior to shift
            registersV[0xF] = instruction >> 15;
            registersV[x] = registersV[y] << 1;
            break;
        }

        // Skip next instruction if Vx != Vy
        case OpCode::SNE_Vx_Vy:
        {
            if (registersV[x] != registersV[y])
            {
                programCounter += 2;
            }
            break;
        }

        // Set I = nnn
        case OpCode::LD_I_NNN:
        {
            registersI = nnn;
            break;
        }

        // Jump to location nnn + V0
        case OpCode::JP_V0_NNN:
        {
            programCounter = nnn + registersV[0];
            break;
        }

        // Set Vx = random byte AND nn
        case OpCode::RND_Vx_NN:
        {
            std::srand(static_cast<unsigned>(std::time(nullptr)));
            auto rand = std::rand() % 256;

            registersV[x] = rand & nn;
            break;
        }

        // Display n-byte sprite starting at memory location I at (Vx, Vy); set VF = collision
        case OpCode::DRW_Vx_Vy_N:
        {
            registersV[0xF] = 0;
            for (auto row = 0; row < n; row++)
            {
                auto spriteByte = mem[registersI + row];
                for (auto bit = 0; bit < 8; bit++)
                {
                    // Process bits left-to-right
                    const auto spritePx = spriteByte & (0x80 >> bit);

                    // Erasing a pixel involves next_frame_readying it again, so only
                    // process pixels that are not 0
                    if (spritePx == 0) continue;

                    auto pxx = registersV[x] + bit;
                    auto pyy = registersV[y] + row;

                    // TODO: Make optional
                    // As per Cowgod's desc, wrap pixels if they overflow
                    // http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#Dxyn
                    if (pxx >= framebuffer.kWidth)  pxx %= framebuffer.kWidth;
                    if (pyy >= framebuffer.kHeight) pyy %= framebuffer.kHeight;

                    auto framePx = framebuffer.pixels + pxx + pyy * framebuffer.kWidth;
                    if (*framePx == 1)
                    {
                        // Vf is used to indicate collisions
                        // It should be set to 1 if any 'on' pixels will be changed to 'off'
                        registersV[0xF] = 1;
                    }

                    // Pixels are drawn by XOR-ing
                    *framePx ^= 1;
                }
            }
            break;
        }

        // Skip next instruction if key with the value of Vx is pressed
        case OpCode::SKP_Vx:
        {
            if (keyState[registersV[x]])
            {
                programCounter += 2;
            }
            break;
        }

        // Skip next instruction if key with the value of Vx is not pressed
        case OpCode::SKNP_Vx:
        {
            if (!keyState[registersV[x]])
            {
                programCounter += 2;
            }
            break;
        }

        // Set Vx = delay timer value
        case OpCode::LD_Vx_DT:
        {
            registersV[x] = registersDelayTimer;
            break;
        }

        // TODO: Inefficient
        // Wait for a key press then store the value of the key in Vx
        case OpCode::LD_Vx_K:
        {
            for (auto i = 0; i < 16; i++)
            {
                if (keyState[i])
                {
                    registersV[x] = i;
                    programCounter += 2;
                    break;
                }
            }
            return;
        }

        // Set delay timer = Vx
        case OpCode::LD_DT_Vx:
        {
            registersDelayTimer = registersV[x];
            break;
        }

        // Set sound timer = Vx
        case OpCode::LD_ST_Vx:
        {
            registersSoundTimer = registersV[x];
            break;
        }

        // Set I = I + Vx
        case OpCode::ADD_I_Vx:
        {
            registersI += registersV[x];
            break;
        }

        // Set I = location of sprite for digit Vx
        case OpCode::LD_F_Vx:
        {
            // Each sprite is 5 bytes and stored contiguously starting at 0x000
            registersI = x * 5;
            break;
        }

        // Store BCD representation of Vx in memory locations I (100), I+1 (10), and I+2 (1)
        case OpCode::LD_B_Vx:
        {
            const auto vx = registersV[x];
            mem[registersI]     = vx / 100;      // 100
            mem[registersI + 1] = vx % 100 / 10; // 10
            mem[registersI + 2] = vx % 100 % 10; // 1
            break;
        }

        // Store registers V0 through Vx in memory starting at I
        case OpCode::LD_I_Vx:
        {
            for (auto j = 0; j <= x; j++, registersI++)
            {
                mem[registersI] = registersV[j];
            }
            break;
        }

        // Read registers V0 through Vx from memory starting at I
        case OpCode::LD_Vx_I:
        {
            for (auto j = 0; j <= x; j++, registersI++)
            {
                registersV[j] = mem[registersI];
            }
            break;
        }

        default: // NOOP
        {
            printf("Unrecognised instruction @ 0x%04x: %04x\n", programCounter, instruction);
            dumpRegisters();
            dumpMemory(32, programCounter);
            exit(1);
        }
    }

    programCounter += 2;
}

void Interpreter::createFontSprites()
{
    // Each digit is represented by 5 bytes
    const u8 hex[] = {
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

    // Copy into mem before PROGRAM_START_ADDR, i.e. starting at 0x000
    memcpy(mem, hex, sizeof(hex));
}

void Interpreter::dumpRegisters() const
{
    printf("registersV:\n");
    for (auto i = 0; i < 16; i++)
    {
        printf("  V%x: %X\n", i, registersV[i]);
    }

    printf("registersI: %X\n", registersI);
    printf("registersDelay: %X\n", registersDelayTimer);
    printf("registersSound: %X\n", registersSoundTimer);
}

void Interpreter::dumpMemory(unsigned bytes, unsigned offset) const
{
    printf("memory:\n");
    for (auto i = offset; i < offset + bytes; i += 2)
    {
        printf("  0x%04x: %02X %02X\n", i, mem[i], mem[i + 1]);
    }
}
