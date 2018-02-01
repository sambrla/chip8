#include <SFML/Graphics.hpp>
#include "chip8.hpp"

#define IPC_TITLE(ipc) "Chip-8 : " + std::to_string(ipc) + " IPC"

#define SCALE   20 // The original Chip-8 res was only 64x32
#define IPC_MIN 0
#define IPC_MAX 100
#define IPC_DEF 9

Chip8::Chip8(Interpreter *vm) :
    vm(vm), win(sf::VideoMode(64 * SCALE, 32 * SCALE), IPC_TITLE(IPC_DEF)), ipc(IPC_DEF)
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

    df.applySettings(s);
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

        sf::Event event;
        while (win.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                win.close();
                return;
            }
        }
        pollKeyInput();

        win.clear();
        drawFrame();
        if (showDF)
        {
            // Log how long this cycle took (in ms)
            // Calling asMilliseconds doesn't produce the desired precision
            df.addDataPoint(timer.getElapsedTime().asMicroseconds() * 0.001f);
            df.drawGraph(&win);
        }
        win.display();

        // Maintain a 60 Hz cadence whenever possible. Will not sleep if n <= 0
        sf::sleep(hz - timer.getElapsedTime());
    }
}

void Chip8::drawFrame()
{
    const auto *frame = vm->frame();
    sf::VertexArray vertices(sf::Quads, frame->kWidth * frame->kHeight * 4);

    for (auto y = 0; y < frame->kHeight; y++)
    {
        // i => pixel index
        for (auto x = 0, i = 0; x < frame->kWidth; x++, i = x + y * frame->kWidth)
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

void Chip8::pollKeyInput()
{
    // Evaluate hex keypad keys
    for (auto const &key : Keymap)
    {
        vm->setKeyState(key.second, sf::Keyboard::isKeyPressed(key.first));
    }

    // F1: reset IPC
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1) && ipc != IPC_DEF)
    {
        ipc = IPC_DEF;
        win.setTitle(IPC_TITLE(ipc));
    }

    // F3: decrease IPC
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F3) && ipc > IPC_MIN)
    {
        win.setTitle(IPC_TITLE(--ipc));
    }

    // F4: increase IPC
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F4) && ipc < IPC_MAX)
    {
        win.setTitle(IPC_TITLE(++ipc));
    }

    // F8: show diag plot
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::F8) && !showDF)
    {
        showDF = true;
    }
    // F9: hide diag plot
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::F9) && showDF)
    {
        showDF = false;
        df.clear();
    }
}
