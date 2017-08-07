#include <iostream>
#include <SFML/Graphics.hpp>
#include "common.hpp"
#include "interpreter.hpp"

#define SCALE 15 // The original Chip-8 resolution was 64x32. Scale that by a factor of ?

void draw_frame(sf::RenderWindow& win, const Interpreter::FrameBuffer* frame)
{
    sf::VertexArray vertices(sf::Quads, frame->kWidth * frame->kHeight * 4);
    for (auto y = 0; y < frame->kHeight; y++)
        for (auto x = 0, i = 0; x < frame->kWidth; x++, i = x + y * frame->kWidth) // i = pixel index
        {
            auto px = frame->pixels + i;
            if ((*px & 1) == 1) // is on
            {
                auto* quad = &vertices[i * 4];

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

    win.draw(vertices);
}

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " path\\to\\rom" << std::endl;
        return 1;
    }

    Interpreter chip8;
    chip8.load_rom(argv[1]);

    sf::RenderWindow win(sf::VideoMode(64 * SCALE, 32 * SCALE), "Chip-8");
    win.setFramerateLimit(60);
    win.setKeyRepeatEnabled(false);

    while (win.isOpen())
    {
        sf::Event event;
        while (win.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                win.close();
            }
            if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased)
            {
                switch (event.key.code)
                {
                    case sf::Keyboard::Num0:
                    {
                        chip8.key_state_changed(Key_0, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::Num1:
                    {
                        chip8.key_state_changed(Key_1, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::Num2:
                    {
                        chip8.key_state_changed(Key_2, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::Num3:
                    {
                        chip8.key_state_changed(Key_3, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::Num4:
                    {
                        chip8.key_state_changed(Key_4, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::Num5:
                    {
                        chip8.key_state_changed(Key_5, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::Num6:
                    {
                        chip8.key_state_changed(Key_6, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::Num7:
                    {
                        chip8.key_state_changed(Key_7, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::Num8:
                    {
                        chip8.key_state_changed(Key_8, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::Num9:
                    {
                        chip8.key_state_changed(Key_9, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::A:
                    {
                        chip8.key_state_changed(Key_A, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::B:
                    {
                        chip8.key_state_changed(Key_B, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::C:
                    {
                        chip8.key_state_changed(Key_C, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::D:
                    {
                        chip8.key_state_changed(Key_D, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::E:
                    {
                        chip8.key_state_changed(Key_E, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    case sf::Keyboard::F:
                    {
                        chip8.key_state_changed(Key_F, sf::Keyboard::isKeyPressed(event.key.code));
                        break;
                    }
                    default:
                    {
                        std::cout << "Unrecognised key " << event.key.code << std::endl;
                        break;
                    }
                }
                std::cout << "Pressed " << event.key.code << std::endl;
            }
        }

        win.clear(sf::Color::Black);

        chip8.cycle();
        draw_frame(win, chip8.frame());

        win.display();
    }
}