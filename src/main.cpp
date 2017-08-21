#include <chrono>
#include <iostream>
#include <map>
#include <thread>
#include <SFML/Graphics.hpp>
#include "interpreter.hpp"

#define SCALE 10 // The original Chip-8 resolution was 64x32. Scale that by a factor of ?
#define EMU_SPEED_HZ 120

typedef std::chrono::steady_clock Clock;

void draw_frame(sf::RenderWindow& win, const Interpreter::FrameBuffer* frame)
{
    sf::VertexArray vertices(sf::Quads, frame->kWidth * frame->kHeight * 4);
    for (auto y = 0; y < frame->kHeight; y++)
        // i = pixel index
        for (auto x = 0, i = 0; x < frame->kWidth; x++, i = x + y * frame->kWidth)
        {
            auto px = frame->pixels + i;

            // Only process 'on' pixels
            if (*px != 1) continue;

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

    win.draw(vertices);
}

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " path\\to\\rom" << std::endl;
        return 1;
    }

    // Map SFML key codes to Chip-8 hex keypad
    const std::map<sf::Keyboard::Key, Interpreter::KeyCode> key_map
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

    Interpreter chip8;
    chip8.load_rom(argv[1]);

    sf::RenderWindow win(sf::VideoMode(64 * SCALE, 32 * SCALE), "Chip-8");
    win.setKeyRepeatEnabled(false);
    win.setTitle("Chip-8 ~ " + std::to_string(EMU_SPEED_HZ) + "Hz");

    // There's little doc on Chip-8 timing save for 60 Hz timers
    // TODO: Allow speed to be modified by key-combo
    const auto timer_refresh_rate = std::chrono::microseconds(1000000 / 60); // 60 Hz
    auto main_refresh_rate = std::chrono::microseconds(1000000 / EMU_SPEED_HZ);

    auto start = Clock::now();
    auto next_timer = start;
    auto next_frame = start;
    sf::Event event;
    for (;;)
    {
        start = Clock::now();
        chip8.cycle();

        // Process input
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
                    auto key = key_map.find(event.key.code);
                    if (key != key_map.end())
                    {
                        chip8.press_key(key->second);
                    }
                    break;
                }
                case sf::Event::KeyReleased:
                {
                    auto key = key_map.find(event.key.code);
                    if (key != key_map.end())
                    {
                        chip8.release_key(key->second);
                    }
                    break;
                }
                default:
                    // Ignore other event types
                    break;
            }
        }

        if (start >= next_timer)
        {
            chip8.cycle_timers();
            next_timer = start + timer_refresh_rate;

            // Draw the screen at the same cadence as the timer refresh, i.e. 60 Hz
            win.clear(sf::Color::Black);
            draw_frame(win, chip8.frame());
            win.display();
        }

        next_frame = start + main_refresh_rate;
        std::this_thread::sleep_until(next_frame);
    }
}