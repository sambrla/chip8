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
    const std::map<sf::Keyboard::Key, Interpreter::KeyCode> Keymap
    {
        { sf::Keyboard::Key::Num1, Interpreter::KeyCode::Key1 },
        { sf::Keyboard::Key::Num2, Interpreter::KeyCode::Key2 },
        { sf::Keyboard::Key::Num3, Interpreter::KeyCode::Key3 },
        { sf::Keyboard::Key::Num4, Interpreter::KeyCode::KeyC },
        { sf::Keyboard::Key::Q,    Interpreter::KeyCode::Key4 },
        { sf::Keyboard::Key::E,    Interpreter::KeyCode::Key5 },
        { sf::Keyboard::Key::A,    Interpreter::KeyCode::Key6 },
        { sf::Keyboard::Key::R,    Interpreter::KeyCode::KeyD },
        { sf::Keyboard::Key::W,    Interpreter::KeyCode::Key7 },
        { sf::Keyboard::Key::S,    Interpreter::KeyCode::Key8 },
        { sf::Keyboard::Key::D,    Interpreter::KeyCode::Key9 },
        { sf::Keyboard::Key::F,    Interpreter::KeyCode::KeyE },
        { sf::Keyboard::Key::Z,    Interpreter::KeyCode::KeyA },
        { sf::Keyboard::Key::X,    Interpreter::KeyCode::Key0 },
        { sf::Keyboard::Key::C,    Interpreter::KeyCode::KeyB },
        { sf::Keyboard::Key::V,    Interpreter::KeyCode::KeyF }
    };

    DF df;
    Interpreter *vm;
    sf::RenderWindow win;

    int ipc; // Instructions per cycle
    bool showDF;

    void pollKeyInput();
    void drawFrame();
};

#endif // CHIP8_H_