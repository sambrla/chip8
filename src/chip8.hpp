#ifndef CHIP8_H_
#define CHIP8_H_

#include <map>
#include <memory>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include "interpreter.hpp"

class Chip8
{
public:
    explicit Chip8(unsigned ipc, bool isHighDpi);
    void run(const std::string& rom, bool withCompatibility);

private:
    // Map SFML key codes to Chip-8 hex keypad
    const std::map<sf::Keyboard::Key, char> Keymap
    {
        { sf::Keyboard::Key::Num1, 0x1 },
        { sf::Keyboard::Key::Num2, 0x2 },
        { sf::Keyboard::Key::Num3, 0x3 },
        { sf::Keyboard::Key::Num4, 0xC },
        { sf::Keyboard::Key::Q,    0x4 },
        { sf::Keyboard::Key::W,    0x5 },
        { sf::Keyboard::Key::E,    0x6 },
        { sf::Keyboard::Key::R,    0xD },
        { sf::Keyboard::Key::A,    0x7 },
        { sf::Keyboard::Key::S,    0x8 },
        { sf::Keyboard::Key::D,    0x9 },
        { sf::Keyboard::Key::F,    0xE },
        { sf::Keyboard::Key::Z,    0xA },
        { sf::Keyboard::Key::X,    0x0 },
        { sf::Keyboard::Key::C,    0xB },
        { sf::Keyboard::Key::V,    0xF }
    };

    Interpreter vm;
    sf::RenderWindow window;
    sf::SoundBuffer buzzerBuffer;
    sf::Sound buzzer;
    unsigned ipc; // Instructions per cycle
    unsigned scale;
    bool isPaused;

    void onKeyDn(const sf::Event& event);
    void onKeyUp(const sf::Event& event);
    void drawFrame();
    void initSound();
};

#endif // CHIP8_H_
