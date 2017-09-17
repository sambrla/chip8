#include <iostream>
#include <map>
#include <thread>
#include <SFML/Graphics.hpp>
#include "interpreter.hpp"

#define IPC_TITLE(ipc) "Chip-8 : " + std::to_string(ipc) + " IPC"

#define SCALE   10 // The original Chip-8 res was only 64x32
#define IPC_MIN 0
#define IPC_MAX 100
#define IPC_DEF 9

// Map SFML key codes to Chip-8 hex keypad
static const std::map<sf::Keyboard::Key, Interpreter::KeyCode> Keymap
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

void drawFrame(sf::RenderWindow* win, const Interpreter::FrameBuffer* frame)
{
    sf::VertexArray vertices(sf::Quads, frame->kWidth * frame->kHeight * 4);
    for (auto y = 0; y < frame->kHeight; y++)
    {
        // i = pixel index
        for (auto x = 0, i = 0; x < frame->kWidth; x++, i = x + y * frame->kWidth)
        {
            auto px = frame->pixels + i;
            if (*px == 1)
            {
                auto quad = &vertices[i * 4];

                // TODO: Scale correctly for high-DPI displays
                quad[0].position = sf::Vector2f(x * SCALE,         y * SCALE); 
                quad[1].position = sf::Vector2f(x * SCALE + SCALE, y * SCALE);
                quad[2].position = sf::Vector2f(x * SCALE + SCALE, y * SCALE + SCALE);
                quad[3].position = sf::Vector2f(x * SCALE,         y * SCALE + SCALE);

                quad[0].color = sf::Color::White;
                quad[1].color = sf::Color::White;
                quad[2].color = sf::Color::White;
                quad[3].color = sf::Color::White;
            }
        }
    }
    win->draw(vertices);
}

int main(int argc, char** argv)
{  
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " path\\to\\rom" << std::endl;
        return 1;
    }

    Interpreter vm;
    vm.load(argv[1]);

    // Instructions per cycle
    auto ipc = IPC_DEF;

    sf::RenderWindow win(sf::VideoMode(64 * SCALE, 32 * SCALE), IPC_TITLE(ipc));
    win.setKeyRepeatEnabled(false);

    sf::Clock timer;
    const auto hz = sf::milliseconds(1000/60); // 60 Hz, 16.6 ms
    for (;;)
    {
        timer.restart();

        for (auto i = 0; i < ipc; i++)
        {
            // Execute a 'CPU' cycle
            vm.cycle();
        }
        vm.cycleTimers();

        // Process input
        sf::Event event;
        while (win.pollEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                {
                    win.close();
                    return 0;
                }
                case sf::Event::KeyPressed:
                {
                    if (event.key.code == sf::Keyboard::F1) // Reset IPC
                    {
                        ipc = IPC_DEF;
                        win.setTitle(IPC_TITLE(ipc));
                    }
                    if (event.key.code == sf::Keyboard::F3) // Decrease IPC
                    {
                        ipc = std::max(--ipc, IPC_MIN);
                        win.setTitle(IPC_TITLE(ipc));
                    }
                    if (event.key.code == sf::Keyboard::F4) // Increase IPC
                    {
                        ipc = std::min(++ipc, IPC_MAX);
                        win.setTitle(IPC_TITLE(ipc));
                    }
                     
                    auto key = Keymap.find(event.key.code);
                    if (key != Keymap.end()) vm.pressKey(key->second);
                    break;
                }
                case sf::Event::KeyReleased:
                {
                    auto key = Keymap.find(event.key.code);
                    if (key != Keymap.end()) vm.releaseKey(key->second);
                    break;
                }
                default:
                    // Ignore other event types
                    break;
            }
        }  
        
        win.clear();
        drawFrame(&win, vm.frame());
        win.display();
            
        // Sleep until next cycle should execute. Will not sleep if n <= 0
        sf::sleep(hz - timer.getElapsedTime());
    }
}
