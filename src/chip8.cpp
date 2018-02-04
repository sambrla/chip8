#include <SFML/Graphics.hpp>
#include "chip8.hpp"

#define WIN_TITLE(ipc) vm->romInfo()->name \
                       + " (" + std::to_string(60*ipc) + " inst/s)"

#define SCALE   15 // The original Chip-8 res was 64x32
#define IPC_MIN 0
#define IPC_MAX 200
#define IPC_STD 9

Chip8::Chip8(Interpreter *vm) :
    vm(vm), ipc(IPC_STD),
    window(sf::VideoMode(64 * SCALE, 32 * SCALE), WIN_TITLE(IPC_STD))
{
    auto s = DF::Settings();
    s.x            = 20;
    s.y            = window.getSize().y - 220;
    s.width        = window.getSize().x - 40;
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
        for (auto i = 0; i < ipc; i++)
        {
            vm->cycle();
        }
        // Timers should be cycled at 60 Hz (once every iteration)
        vm->cycleTimers();

        // Update VM key state
        for (auto const &key : Keymap)
        {
            vm->setKeyState(key.second, sf::Keyboard::isKeyPressed(key.first));
        }

        // Handle non VM specific input
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                case sf::Event::Closed:
                    window.close();
                    return;

                case sf::Event::KeyPressed:
                    // Ctrl+<: decrease IPC
                    if (event.key.control &&
                        event.key.code == sf::Keyboard::Comma &&
                        ipc > IPC_MIN)
                    {
                        ipc--;
                        window.setTitle(WIN_TITLE(ipc));
                    }
                    // Ctrl+>: increase IPC
                    if (event.key.control &&
                        event.key.code == sf::Keyboard::Period &&
                        ipc < IPC_MAX)
                    {
                        ipc++;
                        window.setTitle(WIN_TITLE(ipc));
                    }
                    break;

                case sf::Event::KeyReleased:
                    // Ctrl+0: reset IPC
                    if (event.key.control &&
                        event.key.code == sf::Keyboard::Num0)
                    {
                        ipc = IPC_STD;
                        window.setTitle(WIN_TITLE(ipc));
                    }
                    // Ctrl+D: toggle diagnostic graph
                    if (event.key.control &&
                        event.key.code == sf::Keyboard::D)
                    {
                        showDiag = !showDiag;
                        if (!showDiag) diagGraph.clear();
                    }
                    // Ctrl+R: reset VM
                    if (event.key.control &&
                        event.key.code == sf::Keyboard::R)
                    {
                        vm->reset();
                    }
                    break;

                // Ignore other event types
                default: break;
            }
        }

        window.clear();
        drawFrame();
        if (showDiag)
        {
            // Log how long this cycle took (in ms)
            // Calling asMilliseconds doesn't produce the desired precision
            diagGraph.addDataPoint(
                timer.getElapsedTime().asMicroseconds() * 0.001f);
            diagGraph.draw(&window);
        }
        window.display();

        // Maintain 60 Hz whenever possible. Will not sleep if val <= 0
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
                auto quad = &vertices[i*4];

                quad[0].position = sf::Vector2f(x*SCALE,       y*SCALE);
                quad[1].position = sf::Vector2f(x*SCALE+SCALE, y*SCALE);
                quad[2].position = sf::Vector2f(x*SCALE+SCALE, y*SCALE+SCALE);
                quad[3].position = sf::Vector2f(x*SCALE,       y*SCALE+SCALE);

                quad[0].color = sf::Color::White;
                quad[1].color = sf::Color::White;
                quad[2].color = sf::Color::White;
                quad[3].color = sf::Color::White;
            }
        }
    }
    window.draw(vertices);
}
