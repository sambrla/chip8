#include <iostream>
#include <SFML/Graphics.hpp>
#include "common.hpp"
#include "interpreter.hpp"

#define SCALE 10 // The original Chip-8 resolution was 64x32. Scale that by a factor of ?

void draw_frame(sf::RenderWindow& win, const u8* frame)
{
    sf::VertexArray vertices(sf::Quads, 64 * 32 * 4);

    for (auto y = 0; y < 32; y++)
        for (auto x = 0, i = 0; x < 64; x++, i = (64 * y) + x) // i == pixel offset
        {
            auto px = frame + i;
            if (*px & 1 == 1) // is on
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
    while (win.isOpen())
    {
        sf::Event event;
        while(win.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                win.close();
            }
        }

        win.clear(sf::Color::Black);
        
        chip8.cycle();
        draw_frame(win, chip8.get_frame_buffer());

        win.display();
    }
}