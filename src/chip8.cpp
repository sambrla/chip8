#include <SFML/Graphics.hpp>
#include "chip8.hpp"

#define WIN_TITLE(ipc) vm->romInfo()->name \
                       + " (" + std::to_string(60*ipc) + " inst/s)"

#define SCALE   20 // The original Chip-8 res was 64x32
#define IPC_MIN 0
#define IPC_MAX 100
#define IPC_STD 9

Chip8::Chip8(Interpreter *vm) :
    vm(vm), ipc(IPC_STD),
    win(sf::VideoMode(64 * SCALE, 32 * SCALE), WIN_TITLE(IPC_STD))
{
    auto s = DF::Settings();
    s.x            = 20;
    s.y            = win.getSize().y - 220;
    s.width        = win.getSize().x - 40;
    s.height       = 200;
    s.lineColor    = sf::Color::Cyan;
    s.transparency = 0.8f;
    s.vscale       = 16;
    s.vscaleLabel  = " ms"; // us => \xb5s
    s.fontName     = "OperatorMono-Book.otf";

    diagGraph.applySettings(s);
}

void Chip8::run()
{
    sf::Clock timer;
    const auto hz = sf::milliseconds(1000/60); // 60 Hz, 16.6 ms
    for (;;)
    {
        timer.restart();

        // The IPC value controls the effective emulation speed
        for (auto i = 0; i < ipc; i++)
        {
            vm->cycle();
        }
        // Timers should be cycled at 60 Hz, so once every iteration
        vm->cycleTimers();

        // Update VM key state
        for (auto const &key : Keymap)
        {
            vm->setKeyState(key.second, sf::Keyboard::isKeyPressed(key.first));
        }

        // SFML event handling
        sf::Event event;
        while (win.pollEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                    win.close();
                    return;

                case sf::Event::KeyPressed:
                    // F3: decrease IPC
                    if (event.key.code == sf::Keyboard::F3 && ipc > IPC_MIN)
                    {
                        ipc--;
                        win.setTitle(WIN_TITLE(ipc));
                    }
                    // F4: increase IPC
                    if (event.key.code == sf::Keyboard::F4 && ipc < IPC_MAX)
                    {
                        ipc++;
                        win.setTitle(WIN_TITLE(ipc));
                    }
                    break;

                case sf::Event::KeyReleased:
                    // F1: reset IPC
                    if (event.key.code == sf::Keyboard::F1)
                    {
                        ipc = IPC_STD;
                        win.setTitle(WIN_TITLE(ipc));
                    }
                    // F8: toggle diagnostic graph
                    if (event.key.code == sf::Keyboard::F8)
                    {
                        showDiag = !showDiag;
                        if (!showDiag) diagGraph.clear();
                    }
                    break;

                // Ignore other event types
                default: break;
            }
        }

        win.clear();
        drawFrame();
        if (showDiag)
        {
            // Log how long this cycle took (in ms)
            // Calling asMilliseconds doesn't produce the desired precision
            diagGraph.addDataPoint(timer.getElapsedTime().asMicroseconds() * 0.001f);
            diagGraph.draw(&win);
        }
        win.display();

        // Maintain 60 Hz whenever possible. Will not sleep if n <= 0
        sf::sleep(hz - timer.getElapsedTime());
    }
}

void Chip8::drawFrame()
{
    const auto *frame = vm->frameBuffer();
    sf::VertexArray vertices(sf::Quads, frame->Width * frame->Height * 4);

    for (auto y = 0; y < frame->Height; y++)
    {
        // i = pixel index
        auto i = y * frame->Width;
        for (auto x = 0; x < frame->Width; x++, i = x + y * frame->Width)
        {
            auto px = frame->pixels + i;
            if (*px == 1) // Only process pixels that are 'on'
            {
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
        }
    }
    win.draw(vertices);
}
