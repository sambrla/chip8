#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <string>

#define MEMORY_SIZE 4096

class Interpreter
{
  public:
    using u8  = unsigned char;
    using u16 = unsigned short;

    struct ProgramInfo
    {
        unsigned size;
        std::string name;
        std::string path;
    };

    struct FrameBuffer
    {
        static constexpr u8 Width  = 64;
        static constexpr u8 Height = 32;
        u8 pixels[Width * Height];
    };

    Interpreter();
    void reset();
    bool loadProgram(const std::string& program);
    void cycle();
    void cycleTimers();
    void setKeyState(u8 hexKeyCode, bool isPressed);
    bool isBuzzerSet() const;
    const ProgramInfo& programInfo() const;
    const FrameBuffer& frameBuffer() const;

  private:
    bool keyState[16];
    ProgramInfo progInfo;
    FrameBuffer buffer;

    u8  mem[MEMORY_SIZE]{};
    u8  registersV[16];
    u16 registersI;
    u8  registersST;
    u8  registersDT;
    u8  stackPointer;
    u16 stack[16];
    u16 programCounter;

    void loadFontSprites();
    void execute(u16 instruction);
    void drawToBuffer(u8 x, u8 y, u8 n);
    void dumpRegisters() const;
    void dumpMemory(u8 bytes, u8 offset = 0) const;
};

#endif // INTERPRETER_H_
