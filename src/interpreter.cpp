#include <bitset>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "interpreter.hpp"

Interpreter::Interpreter()
{
    stack[stack_pointer] = program_counter;
    load_font_sprites();
}

void Interpreter::cycle()
{
    const u16 instruction = mem[program_counter] << 8 | mem[program_counter + 1];
    execute_instruction(instruction);

    // TODO: Supposed to count down at a rate of 60Hz.
    if (registers_delay_timer > 0) registers_delay_timer--;

    // TODO: Also counts down at a rate of 60Hz; however, if > 0, plays a sound
    if (registers_sound_timer > 0) registers_sound_timer--;
}

void Interpreter::load_rom(std::string rom_path)
{
    std::ifstream stream(rom_path, std::ios::binary);
    if (stream.is_open())
    {
        stream.seekg(0, stream.end);
        const auto len = stream.tellg();
        stream.seekg(0);
        stream.read(reinterpret_cast<char*>(&mem[PROGRAM_START_ADDR]), len);
    }
    else
    {
        std::cerr << "Unable to open rom file: " << rom_path << std::endl;
    }
    stream.close();
}

const Interpreter::FrameBuffer* Interpreter::frame() const
{
    return &framebuffer;
}

void Interpreter::key_state_changed(Keypad key, bool isPressed)
{
    key_state[key] = isPressed;
}

void Interpreter::execute_instruction(u16 instruction)
{
    switch (opcode(instruction))
    {
        // Fx1E: Set reg_I = reg_I + Vx
        case OpCode::ADD_I_Vx:
        {
            auto x = (instruction & 0x0F00) >> 8;
            registers_i += registers_v[x];
            break;
        }

        // 7xnn: Set Vx to Vx + nn
        case OpCode::ADD_Vx_NN:
        {
            auto x = (instruction & 0x0F00) >> 8;
            registers_v[x] += instruction & 0x00FF;
            break;
        }

        // 8xy4: Set Vx to Vx + Vy. If result > 255, Vf = 1; else 0
        case OpCode::ADD_Vx_Vy:
        {
            auto x = (instruction & 0x0F00) >> 8;
            auto y = (instruction & 0x00F0) >> 4;

            auto sum = x + y;
            registers_v[0xF] = sum > 255 ? 1 : 0;
            registers_v[x] = static_cast<u8>(sum);
            break;
        }

        // 8xy2: Set Vx to Vx AND Vy
        case OpCode::AND_Vx_Vy:
        {
            auto x = (instruction & 0x0F00) >> 8;
            auto y = (instruction & 0x00F0) >> 4;
            registers_v[x] = x & y;
            break;
        }

        // Fx33: Store binary-coded decimal of Vx in mem at addr reg_I (100), reg_I+1 (10), reg_I+2 (1)
        case OpCode::BCD_Vx:
        {
            auto  x = (instruction & 0x0F00) >> 8;
            auto vx = registers_v[x];

            mem[registers_i] = vx / 100;    // 100
            vx %= 100;
            mem[registers_i + 1] = vx / 10; // 10
            mem[registers_i + 2] = vx % 10; // 1
            break;
        }

        // 2nnn: Call subroutine at nnn
        case OpCode::CALL_NNN:
        {
            stack_pointer++;
            stack[stack_pointer] = program_counter;
            program_counter = instruction & 0x0FFF;
            return; // Don't want the pc to increment
        }

        // Dxyn: Draw sprite at Vx,Vy with n bytes of sprite data starting at reg_I
        case OpCode::DRW_Vx_Vy_N:
        {
            const auto x = registers_v[(instruction & 0x0F00) >> 8];
            const auto y = registers_v[(instruction & 0x00F0) >> 4];
            const auto n = instruction & 0x000F;

            registers_v[0xF] = 0;
            for (auto i = 0; i < n; i++)
            {
                auto sprite_byte = mem[registers_i + i];
                for (auto j = 0; j < 8; j++)
                {
                    // Process bits left-to-right
                    const auto sprite_px = sprite_byte & (0x80 >> j);

                    // Erasing a pixel involves drawing it again, so only process pixels that are not 0
                    if (sprite_px != 0)
                    {
                        auto px_x = x + j;
                        auto px_y = y + i;

                        // As per Cowgod's desc, wrap pixels if they overflow past frame bounds
                        // http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#Dxyn
                        if (px_x >= framebuffer.kWidth)  px_x %= framebuffer.kWidth;
                        if (px_y >= framebuffer.kHeight) px_y %= framebuffer.kHeight;

                        auto frame_px = framebuffer.pixels + px_x + px_y * framebuffer.kWidth;

                        // Vf is used to indicate collisions
                        // It should be set to 1 if any 'on' pixels will be changed to 'off'
                        if (*frame_px == 1) registers_v[0xF] = 1;

                        // Pixels are drawn by XOR-ing
                        *frame_px ^= 1;
                    }
                }
            }
            break;
        }

        // 1nnn: Jump to addr nnn
        case OpCode::JMP_NNN:
        {
            program_counter = instruction & 0x0FFF;
            return; // Don't want the pc to increment
        }

        // Fx15: Load DT with Vx
        case OpCode::LD_DT_Vx:
        {
            auto x = (instruction & 0x0F00) >> 8;
            registers_delay_timer = registers_v[x];
            break;
        }

        // Fx29: Set reg_I to mem addr of font sprite referred to in Vx
        case OpCode::LD_F_Vx:
        {
            auto x = (instruction & 0x0F00) >> 8;
            registers_i = x * 5; // Each sprite is 5 bytes and stored in mem contiguously starting at 0x000
            break;
        }

        // Annn: Store nnn in reg I
        case OpCode::LD_I_NNN:
        {
            registers_i = instruction & 0x0FFF;
            break;
        }

        // Fx55: Load mem, starting at reg_I, with registers V0..Vx (inc.)
        case OpCode::LD_I_Vx:
        {
            auto x = (instruction & 0x0F00) >> 8;
            for (auto i = 0; i <= x; i++, registers_i++)
            {
                mem[registers_i] = registers_v[i];
            }
            break;
        }
        
        // Fx18: Set reg_ST = Vx
        case OpCode::LD_ST_Vx:
        {
            auto x = (instruction & 0x0F00) >> 8;
            registers_sound_timer = registers_v[x];
            break;
        }

        // Fx07: Load Vx with DT
        case OpCode::LD_Vx_DT:
        {
            auto x = (instruction & 0x0F00) >> 8;
            registers_v[x] = registers_delay_timer;
            break;
        }

        // Fx65: Load registers V0..Vx (inc.) with values stored in mem starting at reg_I
        case OpCode::LD_Vx_I:
        {
            auto x = (instruction & 0x0F00) >> 8;
            for (auto i = 0; i <= x; i++, registers_i++)
            {
                registers_v[i] = mem[registers_i];
            }
            break;
        }

        // 6xnn: Load Vx with nn
        case OpCode::LD_Vx_NN:
        {
            auto x = (instruction & 0x0F00) >> 8;
            registers_v[x] = instruction & 0x00FF;
            break;
        }

        // 8xy0: Load Vx with Vy
        case OpCode::LD_Vx_Vy:
        {
            auto x = (instruction & 0x0F00) >> 8;
            auto y = (instruction & 0x00F0) >> 4;
            registers_v[x] = registers_v[y];
            break;
        }

        // Return
        case OpCode::RET:
        {
            program_counter = stack[stack_pointer];
            stack_pointer--;
            break;
        }

        // Cxnn: Set Vx with random number between 0-255 AND nn
        case OpCode::RND_Vx_NN:
        {
            std::srand(static_cast<unsigned>(std::time(nullptr)));
            auto rand = std::rand() % 256;

            auto x = (instruction & 0x0F00) >> 8;
            registers_v[x] = rand & (instruction & 0x00FF);
            break;
        }

        // 3xnn: Skip next instruction if Vx == nn
        case OpCode::SE_Vx_NN:
        {
            auto x = (instruction & 0x0F00) >> 8;
            if (registers_v[x] == (instruction & 0x00FF))
            {
                program_counter += 2;
            }
            break;
        }

        // Ex9E: Skip next instruction if key(Vx) is pressed
        case OpCode::SKP_Vx:
        {
            auto x = (instruction & 0x0F00) >> 8;
            if (key_state[x])
            {
                program_counter += 2;
            }
            // printf("Unrecognised instruction @ %#x: %x\n", program_counter, instruction);
            // dump_registers();
            // dump_memory(32, registers_i);
            // exit(1);
            break;
        }

        // ExA1: Skip next instruction if key(Vx) is not pressed
        case OpCode::SKNP_Vx:
        {
            auto x = (instruction & 0x0F00) >> 8;
            if (!key_state[x])
            {
                program_counter += 2;
            }
            // printf("Unrecognised instruction @ %#x: %x\n", program_counter, instruction);
            // dump_registers();
            // dump_memory(32, registers_i);
            // exit(1);
            break;
        }

        // 4xnn: Skip next instruction if Vx != nn
        case OpCode::SNE_Vx_NN:
        {
            auto x = (instruction & 0x0F00) >> 8;
            if (registers_v[x] != (instruction & 0x00FF))
            {
                program_counter += 2;
            }
            break;
        }

        // 9xy0: Skip next instruction if Vx != Vy
        case OpCode::SNE_Vx_Vy:
        {
            auto x = (instruction & 0x0F00) >> 8;
            auto y = (instruction & 0x00F0) >> 4;
            if (registers_v[x] != registers_v[y])
            {
                program_counter += 2;
            }
            break;
        }

        // 8xy5: Vx = Vx - Vy. Vf = !borrow
        case OpCode::SUB_Vx_Vy:
        {
            auto x = (instruction & 0x0F00) >> 8;
            auto y = (instruction & 0x00F0) >> 4;
            
            registers_v[0xF] = x > y ? 1 : 0;
            registers_v[x]   = x - y;
            break;
        }

        default: // NOOP
        {
            printf("Unrecognised instruction @ %#x: %x\n", program_counter, instruction);
            dump_registers();
            dump_memory(32, registers_i);
            exit(1);
        }
    }

    //printf("%#x: %04x\n", program_counter, instruction);
    program_counter += 2;
}

OpCode Interpreter::opcode(u16 instruction) const
{
    if ((instruction &  0xF0FF) == 0xF01E) return OpCode::ADD_I_Vx;
    if ((instruction &  0xF000) == 0x7000) return OpCode::ADD_Vx_NN;
    if ((instruction &  0xF00F) == 0x8004) return OpCode::ADD_Vx_Vy;
    if ((instruction &  0xF00F) == 0x8002) return OpCode::AND_Vx_Vy;
    if ((instruction &  0xF0FF) == 0xF033) return OpCode::BCD_Vx;
    if ((instruction &  0xF000) == 0x2000) return OpCode::CALL_NNN;
    if ((instruction &  0xF000) == 0xD000) return OpCode::DRW_Vx_Vy_N;
    if ((instruction &  0xF000) == 0x1000) return OpCode::JMP_NNN;
    if ((instruction &  0xF0FF) == 0xF015) return OpCode::LD_DT_Vx;
    if ((instruction &  0xF0FF) == 0xF029) return OpCode::LD_F_Vx;
    if ((instruction &  0xF000) == 0xA000) return OpCode::LD_I_NNN;
    if ((instruction &  0xF0FF) == 0xF055) return OpCode::LD_I_Vx;
    if ((instruction &  0xF0FF) == 0xF018) return OpCode::LD_ST_Vx;
    if ((instruction &  0xF0FF) == 0xF007) return OpCode::LD_Vx_DT;
    if ((instruction &  0xF0FF) == 0xF065) return OpCode::LD_Vx_I;
    if ((instruction &  0xF000) == 0x6000) return OpCode::LD_Vx_NN;
    if ((instruction &  0xF00F) == 0x8000) return OpCode::LD_Vx_Vy;
    if  (instruction == 0x00EE)            return OpCode::RET;
    if ((instruction &  0xF000) == 0xC000) return OpCode::RND_Vx_NN;
    if ((instruction &  0xF000) == 0x3000) return OpCode::SE_Vx_NN;
    if ((instruction &  0xF0FF) == 0xE09E) return OpCode::SKP_Vx;
    if ((instruction &  0xF0FF) == 0xE0A1) return OpCode::SKNP_Vx;
    if ((instruction &  0xF000) == 0x4000) return OpCode::SNE_Vx_NN;
    if ((instruction &  0xF00F) == 0x9000) return OpCode::SNE_Vx_Vy;
    if ((instruction &  0xF00F) == 0x8005) return OpCode::SUB_Vx_Vy;

    // Although there is no NOOP opcode, we'll use it to signal an invalid instruction
    return OpCode::NOOP;
}

void Interpreter::load_font_sprites()
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

void Interpreter::dump_registers() const
{
    printf("registers_v:\n");
    for (auto i = 0; i < 16; i++)
    {
        printf("  V%x: %X\n", i, registers_v[i]);
    }

    printf("registers_i: %X\n", registers_i);
    printf("registers_delay: %X\n", registers_delay_timer);
    printf("registers_sound: %X\n", registers_sound_timer);
}

void Interpreter::dump_memory(unsigned bytes, unsigned offset) const
{
    printf("memory:\n");
    for (auto i = offset; i < offset + bytes; i += 2)
    {
        printf("  %05x: %02X %02X\n", i, mem[i], mem[i + 1]);
    }
}