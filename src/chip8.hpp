#ifndef CHIP8_H_
#define CHIP8_H_

#include <map>
#include <SFML/Window.hpp>
#include "df.hpp"
#include "interpreter.hpp"

class Chip8
{
public:
    explicit Chip8(Interpreter *vm);
    void run();

private:
    // Map SFML key codes to Chip-8 hex keypad
    const std::map<sf::Keyboard::Key, Interpreter::Keypad> Keymap
    {
        { sf::Keyboard::Key::Num1, Interpreter::Keypad::Key1 },
        { sf::Keyboard::Key::Num2, Interpreter::Keypad::Key2 },
        { sf::Keyboard::Key::Num3, Interpreter::Keypad::Key3 },
        { sf::Keyboard::Key::Num4, Interpreter::Keypad::KeyC },
        { sf::Keyboard::Key::Q,    Interpreter::Keypad::Key4 },
        { sf::Keyboard::Key::E,    Interpreter::Keypad::Key6 },
        { sf::Keyboard::Key::A,    Interpreter::Keypad::Key7 },
        { sf::Keyboard::Key::R,    Interpreter::Keypad::KeyD },
        { sf::Keyboard::Key::W,    Interpreter::Keypad::Key5 },
        { sf::Keyboard::Key::S,    Interpreter::Keypad::Key8 },
        { sf::Keyboard::Key::D,    Interpreter::Keypad::Key9 },
        { sf::Keyboard::Key::F,    Interpreter::Keypad::KeyE },
        { sf::Keyboard::Key::Z,    Interpreter::Keypad::KeyA },
        { sf::Keyboard::Key::X,    Interpreter::Keypad::Key0 },
        { sf::Keyboard::Key::C,    Interpreter::Keypad::KeyB },
        { sf::Keyboard::Key::V,    Interpreter::Keypad::KeyF }
    };

    Interpreter *vm;
    DF diagGraph;
    sf::RenderWindow window;
    bool showDiag = false;
    unsigned ipc; // Instructions per cycle

    void drawFrame();
};

#endif // CHIP8_H_
