#include <iostream>
#include <map>
#include <SFML/Graphics.hpp>
#include "interpreter.hpp"

// The original Chip-8 resolution was 64x32. Scale that by a factor of ?
#define SCALE 10

void draw_frame(sf::RenderWindow& win, const Interpreter::FrameBuffer* frame)
{
    sf::VertexArray vertices(sf::Quads, frame->kWidth * frame->kHeight * 4);
    for (auto y = 0; y < frame->kHeight; y++)
        // i = pixel index
        for (auto x = 0, i = 0; x < frame->kWidth; x++, i = x + y * frame->kWidth)
        {
            auto px = frame->pixels + i;
            if (*px != 1) continue;
    
            auto quad = &vertices[i * 4];

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
    const std::map<sf::Keyboard::Key, unsigned char> key_map
    {
        { sf::Keyboard::Key::Num1, 0x1 },
        { sf::Keyboard::Key::Num2, 0x2 },
        { sf::Keyboard::Key::Num3, 0x3 },
        { sf::Keyboard::Key::Num4, 0xC },
        { sf::Keyboard::Key::Q,    0x4 },
        { sf::Keyboard::Key::E,    0x5 },
        { sf::Keyboard::Key::A,    0x6 },
        { sf::Keyboard::Key::R,    0xD },
        { sf::Keyboard::Key::W,    0x7 },
        { sf::Keyboard::Key::S,    0x8 },
        { sf::Keyboard::Key::D,    0x9 },
        { sf::Keyboard::Key::F,    0xE },
        { sf::Keyboard::Key::Z,    0xA },
        { sf::Keyboard::Key::X,    0x0 },
        { sf::Keyboard::Key::C,    0xB },
        { sf::Keyboard::Key::V,    0xF }
    };

    Interpreter chip8;
    chip8.load_rom(argv[1]);
    
    sf::RenderWindow win(sf::VideoMode(64*SCALE, 32*SCALE), "Chip-8");
    win.setFramerateLimit(60); // TODO: fix speed implementation
    win.setKeyRepeatEnabled(false);

    while (win.isOpen())
    {
        sf::Event event;
        while (win.pollEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                {
                    win.close();
                    break;
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
                    break;
            }
        }

        win.clear(sf::Color::Black);
        chip8.cycle();
        draw_frame(win, chip8.frame());
        win.display();
    }
}