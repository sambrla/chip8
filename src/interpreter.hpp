#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <string>

#define MEMORY_SIZE 4096
#define PROGRAM_START_ADDR 0x200 // Most progs start at 0x200 (512)

class Interpreter
{
  public:
    typedef unsigned char   u8;
    typedef unsigned short u16;

    enum KeyCode
    {
        Key0 = 0,
        Key1,
        Key2,
        Key3,
        Key4,
        Key5,
        Key6,
        Key7,
        Key8,
        Key9,
        KeyA,
        KeyB,
        KeyC,
        KeyD,
        KeyE,
        KeyF
    };

    struct FrameBuffer
    {
        static constexpr unsigned kWidth  = 64;
        static constexpr unsigned kHeight = 32;
        u8 pixels[kWidth * kHeight]{};
    };

    Interpreter();

    // Reset interpreter state
    void reset();

    // Execute a single 'cpu' cycle
    void cycle();

    // Update sound and delay timers. This should occur at 60 Hz
    void cycleTimers();

    // Load rom into interpreter memory
    void load(std::string romPath);

    // Hex keypad key handling
    void setKeyState(KeyCode key, bool isPressed);

    // TODO: Implement
    bool shouldBeep() const;

    // Current framebuffer content
    const FrameBuffer* frame() const;

    // Debugging
    void dumpRegisters() const;
    void dumpMemory(unsigned bytes, unsigned offset = 0) const;

  private:
    enum class OpCode
    {
        SYS_NNN,
        CLS,
        RET,
        JP_NNN,
        CALL_NNN,
        SE_Vx_NN,
        SNE_Vx_NN,
        SE_Vx_Vy,
        LD_Vx_NN,
        ADD_Vx_NN,
        LD_Vx_Vy,
        OR_Vx_Vy,
        AND_Vx_Vy,
        XOR_Vx_Vy,
        ADD_Vx_Vy,
        SUB_Vx_Vy,
        SHR_Vx,
        SUBN_Vx_Vy,
        SHL_Vx,
        SNE_Vx_Vy,
        LD_I_NNN,
        JP_V0_NNN,
        RND_Vx_NN,
        DRW_Vx_Vy_N,
        SKP_Vx,
        SKNP_Vx,
        LD_Vx_DT,
        LD_Vx_K,
        LD_DT_Vx,
        LD_ST_Vx,
        ADD_I_Vx,
        LD_F_Vx,
        LD_B_Vx,
        LD_I_Vx,
        LD_Vx_I,
        NOOP
    };

     u8 mem[MEMORY_SIZE]{/* value init */};
     u8 registersV[16]{};
    u16 registersI = 0;
     u8 registersSoundTimer = 0;
     u8 registersDelayTimer = 0;
    u16 stack[16]{};
     u8 stackPointer = 0;
    u16 programCounter = PROGRAM_START_ADDR;

    FrameBuffer framebuffer;
    bool keyState[16]{};

    OpCode opcode(u16 instruction) const;
    void executeInstruction(u16 instruction);
    void createFontSprites();
};

#endif // INTERPRETER_H_
