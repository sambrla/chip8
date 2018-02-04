#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <string>

#define MEMORY_SIZE 4096
#define PROG_START_ADDR 0x200 // Most progs start at 0x200 (512)

class Interpreter
{
  public:
    typedef unsigned char  u8;
    typedef unsigned short u16;

    enum Keypad
    {
        Key0,
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

    struct RomInfo
    {
        unsigned size;
        std::string name;
        std::string path;
    };

    struct FrameBuffer
    {
        static constexpr unsigned Width  = 64;
        static constexpr unsigned Height = 32;
        u8 pixels[Width * Height]{};
    };

    Interpreter();
    void reset();
    void loadRom(std::string path);
    void cycle();
    void cycleTimers();
    void setKeyState(Keypad key, bool isPressed);
    bool beepTriggered() const;
    const RomInfo* romInfo() const;
    const FrameBuffer* frameBuffer() const;

  private:
    bool keyState[16]{};
    RomInfo info;
    FrameBuffer buffer;

    u8  mem[MEMORY_SIZE]{};
    u8  registersV[16]{};
    u16 registersI  = 0;
    u8  registersST = 0;
    u8  registersDT = 0;
    u16 stack[16]{};
    u8  stackPointer = 0;
    u16 programCounter = PROG_START_ADDR;

    void loadFontSprites();
    void execute(u16 instruction);
    void drawToBuffer(u8 x, u8 y, u8 n);
    void dumpRegisters() const;
    void dumpMemory(unsigned bytes, unsigned offset = 0) const;
};

#endif // INTERPRETER_H_
