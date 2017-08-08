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

void Interpreter::press_key(u8 keycode)
{
    if (keycode <= 0xF)
    {
        key_state[keycode] = true;
    }
}

void Interpreter::release_key(u8 keycode)
{
    if (keycode <= 0xF)
    {
        key_state[keycode] = false;
    }
}

bool Interpreter::buzzer() const
{
    return registers_sound_timer > 0;
}

const Interpreter::FrameBuffer* Interpreter::frame() const
{
    return &framebuffer;
}

void Interpreter::execute_instruction(u16 instruction)
{
    const auto x   = (instruction & 0x0F00) >> 8;
    const auto y   = (instruction & 0x00F0) >> 4;
    const auto nnn =  instruction & 0x0FFF;
    const auto kk  =  instruction & 0x00FF;
    const auto n   =  instruction & 0x000F;
    
    switch (opcode(instruction))
    {
        // 00E0: Clear the display
        case OpCode::CLS:
        {
            const auto size = framebuffer.kWidth * framebuffer.kHeight;
            for (auto i = 0; i < size; i++)
            {
                framebuffer.pixels[i] = 0;
            }
            break;
        }

        // 00EE: Return from subroutine
        case OpCode::RET:
        {
            program_counter = stack[stack_pointer];
            stack_pointer--;
            break;
        }

        // 1nnn: Jump to location nnn
        case OpCode::JP_NNN:
        {
            program_counter = nnn;
            return; // Don't want the pc to increment
        }

        // 2nnn: Call subroutine at nnn
        case OpCode::CALL_NNN:
        {
            stack_pointer++;
            stack[stack_pointer] = program_counter;
            program_counter = nnn;
            return; // Don't want the pc to increment
        }

        // 3xkk: Skip next instruction if Vx == kk
        case OpCode::SE_Vx_KK:
        {
            if (registers_v[x] == kk)
            {
                program_counter += 2;
            }
            break;
        }

        // 4xkk: Skip next instruction if Vx != kk
        case OpCode::SNE_Vx_KK:
        {
            if (registers_v[x] != kk)
            {
                program_counter += 2;
            }
            break;
        }

        // 5xy0: Skip next instruction if Vx == Vy
        case OpCode::SE_Vx_Vy:
        {
            if (registers_v[x] == registers_v[y])
            {
                program_counter += 2;
            }
            break;
        }

        // 6xkk: Set Vx = kk
        case OpCode::LD_Vx_KK:
        {
            registers_v[x] = kk;
            break;
        }

        // 7xkk: Set Vx = Vx + kk
        case OpCode::ADD_Vx_KK:
        {
            registers_v[x] += kk;
            break;
        }

        // 8xy0: Set Vx = Vy
        case OpCode::LD_Vx_Vy:
        {
            registers_v[x] = registers_v[y];
            break;
        }

        // 8xy1: Set Vx = Vx OR Vy
        case OpCode::OR_Vx_Vy:
        {
            registers_v[x] |= registers_v[y];
            break;
        }

        // 8xy2: Set Vx = Vx AND Vy
        case OpCode::AND_Vx_Vy:
        {
            registers_v[x] &= registers_v[y];
            break;
        }

        // 8xy3: Set Vx = Vx XOR Vy
        case OpCode::XOR_Vx_Vy:
        {
            registers_v[x] ^= registers_v[y];
            break;
        }

        // 8xy4: Set Vx = Vx + Vy, set VF = carry
        case OpCode::ADD_Vx_Vy:
        {
            auto sum = registers_v[x] + registers_v[y];
            registers_v[0xF] = sum > 255 ? 1 : 0;
            registers_v[x]   = static_cast<u8>(sum);
            break;
        }

        // 8xy5: Set Vx = Vx - Vy, set VF = NOT borrow
        case OpCode::SUB_Vx_Vy:
        {
            registers_v[0xF] = registers_v[x] > registers_v[y] ? 1 : 0;
            registers_v[x]  -= registers_v[y];
            break;
        }

        // 8xy6: Set Vx = Vx SHR 1
        case OpCode::SHR_Vx:
        {
            registers_v[x] = registers_v[x] >> 1;
            break;
        }

        // 8xy7: Set Vx = Vy - Vx, set VF = NOT borrow
        case OpCode::SUBN_Vx_Vy:
        {
            registers_v[0xF] = registers_v[y] > registers_v[x] ? 1 : 0;
            registers_v[x]   = registers_v[y] - registers_v[x];
            break;
        }

        // 8xyE: Set Vx = Vx SHL 1
        case OpCode::SHL_Vx:
        {
            registers_v[x] = registers_v[x] << 1;
            break;
        }

        // 9xy0: Skip next instruction if Vx != Vy
        case OpCode::SNE_Vx_Vy:
        {
            if (registers_v[x] != registers_v[y])
            {
                program_counter += 2;
            }
            break;
        }

        // Annn: Set I = nnn
        case OpCode::LD_I_NNN:
        {
            registers_i = nnn;
            break;
        }

        // Bnnn: Jump to location nnn + V0
        case OpCode::JP_V0_NNN:
        {
            program_counter = nnn + registers_v[0];
            break;
        }

        // Cxkk: Set Vx = random byte AND kk
        case OpCode::RND_Vx_KK:
        {
            std::srand(static_cast<unsigned>(std::time(nullptr)));
            auto rand = std::rand() % 256;

            registers_v[x] = rand & kk;
            break;
        }

        // Dxyn: Display n-byte sprite starting at memory location I at (Vx, Vy); set VF = collision
        case OpCode::DRW_Vx_Vy_N:
        {
            registers_v[0xF] = 0;
            for (auto row = 0; row < n; row++)
            {
                auto sprite_byte = mem[registers_i + row];
                for (auto bit = 0; bit < 8; bit++)
                {
                    // Process bits left-to-right
                    const auto sprite_px = sprite_byte & (0x80 >> bit);

                    // Erasing a pixel involves drawing it again, so only process pixels that are not 0
                    if (sprite_px != 0)
                    {
                        auto px_x = registers_v[x] + bit;
                        auto px_y = registers_v[y] + row;

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

        // Ex9E: Skip next instruction if key with the value of Vx is pressed
        case OpCode::SKP_Vx:
        {
            if (key_state[registers_v[x]])
            {
                program_counter += 2;
            }
            break;
        }

        // ExA1: Skip next instruction if key with the value of Vx is not pressed
        case OpCode::SKNP_Vx:
        {
            if (!key_state[registers_v[x]])
            {
                program_counter += 2;
            }
            break;
        }

        // Fx07: Set Vx = delay timer value
        case OpCode::LD_Vx_DT:
        {
            registers_v[x] = registers_delay_timer;
            break;
        }

        // Fx0A: Wait for a key press, store the value of the key in Vx
        //case OpCode::LD_Vx_K:
        //{
        //    // TODO: Implement. Remove from default:
        //    break;
        //}

        // Fx15: Set delay timer = Vx
        case OpCode::LD_DT_Vx:
        {
            registers_delay_timer = registers_v[x];
            break;
        }

        // Fx18: Set sound timer = Vx
        case OpCode::LD_ST_Vx:
        {
            registers_sound_timer = registers_v[x];
            break;
        }
        
        // Fx1E: Set I = I + Vx
        case OpCode::ADD_I_Vx:
        {
            registers_i += registers_v[x];
            break;
        }

        // Fx29: Set I = location of sprite for digit Vx
        case OpCode::LD_F_Vx:
        {
            // Each sprite is 5 bytes and stored contiguously starting at 0x000
            registers_i = x * 5; 
            break;
        }
        
        // Fx33: Store BCD representation of Vx in memory locations
        // I (100), I+1 (10), and I+2 (1)
        case OpCode::LD_B_Vx:
        {
            const auto vx = registers_v[x];
            mem[registers_i]     = vx / 100;      // 100
            mem[registers_i + 1] = vx % 100 / 10; // 10
            mem[registers_i + 2] = vx % 100 % 10; // 1
            break;
        }

        // Fx55: Store registers V0 through Vx in memory starting at I
        case OpCode::LD_I_Vx:
        {
            for (auto i = 0; i <= x; i++, registers_i++)
            {
                mem[registers_i] = registers_v[i];
            }
            break;
        }
        
        // Fx65: Read registers V0 through Vx from memory starting at I
        case OpCode::LD_Vx_I:
        {
            for (auto i = 0; i <= x; i++, registers_i++)
            {
                registers_v[i] = mem[registers_i];
            }
            break;
        }

        case OpCode::LD_Vx_K:
        default: // NOOP
        {
            printf("Unrecognised instruction @ 0x%04x: %04x\n", program_counter, instruction);
            dump_registers();
            dump_memory(32, program_counter);
            exit(1);
        }
    }

    printf("0x%04x: %04x\n", program_counter, instruction);
    program_counter += 2;
}

OpCode Interpreter::opcode(u16 instruction) const
{
    if  (instruction == 0x00E0)            return OpCode::CLS;
    if  (instruction == 0x00EE)            return OpCode::RET;
    if ((instruction &  0xF000) == 0x1000) return OpCode::JP_NNN;
    if ((instruction &  0xF000) == 0x2000) return OpCode::CALL_NNN;
    if ((instruction &  0xF000) == 0x3000) return OpCode::SE_Vx_KK;
    if ((instruction &  0xF000) == 0x4000) return OpCode::SNE_Vx_KK;
    if ((instruction &  0xF00F) == 0x5000) return OpCode::SE_Vx_Vy;
    if ((instruction &  0xF000) == 0x6000) return OpCode::LD_Vx_KK;
    if ((instruction &  0xF000) == 0x7000) return OpCode::ADD_Vx_KK;
    if ((instruction &  0xF00F) == 0x8000) return OpCode::LD_Vx_Vy;
    if ((instruction &  0xF00F) == 0x8001) return OpCode::OR_Vx_Vy;
    if ((instruction &  0xF00F) == 0x8002) return OpCode::AND_Vx_Vy;
    if ((instruction &  0xF00F) == 0x8003) return OpCode::XOR_Vx_Vy;
    if ((instruction &  0xF00F) == 0x8004) return OpCode::ADD_Vx_Vy;
    if ((instruction &  0xF00F) == 0x8005) return OpCode::SUB_Vx_Vy;
    if ((instruction &  0xF00F) == 0x8006) return OpCode::SHR_Vx;
    if ((instruction &  0xF00F) == 0x8007) return OpCode::SUBN_Vx_Vy;
    if ((instruction &  0xF00F) == 0x800E) return OpCode::SHL_Vx;
    if ((instruction &  0xF00F) == 0x9000) return OpCode::SNE_Vx_Vy;
    if ((instruction &  0xF000) == 0xA000) return OpCode::LD_I_NNN;
    if ((instruction &  0xF000) == 0xB000) return OpCode::JP_V0_NNN;
    if ((instruction &  0xF000) == 0xC000) return OpCode::RND_Vx_KK;
    if ((instruction &  0xF000) == 0xD000) return OpCode::DRW_Vx_Vy_N;
    if ((instruction &  0xF0FF) == 0xE09E) return OpCode::SKP_Vx;
    if ((instruction &  0xF0FF) == 0xE0A1) return OpCode::SKNP_Vx;
    if ((instruction &  0xF0FF) == 0xF007) return OpCode::LD_Vx_DT;
    if ((instruction &  0xF0FF) == 0xF00A) return OpCode::LD_Vx_K;
    if ((instruction &  0xF0FF) == 0xF015) return OpCode::LD_DT_Vx;
    if ((instruction &  0xF0FF) == 0xF018) return OpCode::LD_ST_Vx;
    if ((instruction &  0xF0FF) == 0xF01E) return OpCode::ADD_I_Vx;
    if ((instruction &  0xF0FF) == 0xF029) return OpCode::LD_F_Vx;
    if ((instruction &  0xF0FF) == 0xF033) return OpCode::LD_B_Vx;
    if ((instruction &  0xF0FF) == 0xF055) return OpCode::LD_I_Vx;
    if ((instruction &  0xF0FF) == 0xF065) return OpCode::LD_Vx_I;

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
        printf("  0x%04x: %02X %02X\n", i, mem[i], mem[i + 1]);
    }
}