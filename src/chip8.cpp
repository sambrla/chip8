#include <SFML/Graphics.hpp>
#include "chip8.hpp"

Chip8::Chip8(unsigned ipc, unsigned scale, bool useProfiler)
    : ipc(ipc), scale(scale), isPaused(false)
{
    window.create(
        sf::VideoMode(64*scale, 32*scale + (useProfiler ? 200 : 0)), "Chip-8");

    if (useProfiler)
    {
        auto s = DF::Settings();
        s.x            = 0;
        s.y            = window.getSize().y - 200;
        s.width        = window.getSize().x;
        s.height       = 200;
        s.lineColor    = sf::Color::Cyan;
        s.overColor    = sf::Color::Blue;
        s.vscale       = 16;
        s.vscaleLabel  = " ms";
        // github.com/adobe-fonts/source-code-pro
        s.fontName     = "SourceCodePro-Regular.otf";

        profiler = std::unique_ptr<DF>(new DF(s));
    }
}

void Chip8::run(const std::string& rom)
{
    if (!vm.loadProgram(rom)) return;
    window.setTitle(vm.programInfo().name);

    const auto hz = sf::milliseconds(1000/60); // 60 Hz, 16.6 ms
    sf::Clock timer;
    for (;;)
    {
        timer.restart();

        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                return;

            case sf::Event::KeyPressed:
                onKeyDn(event);
                break;

            case sf::Event::KeyReleased:
                onKeyUp(event);
                break;

            default:
                // Ignore other event types
                break;
            }
        }

        if (isPaused) goto sleep;

        // IPC controls effective emulation speed, i.e. ipc*60 = inst/s
        for (auto i = 0; i < ipc; i++)
        {
            vm.cycle();
        }

        // Update timers at 60 Hz independent of IPC
        vm.cycleTimers();

        window.clear();
        drawFrame();
        if (profiler)
        {
            // Log how long this cycle took (in ms)
            // Calling asMilliseconds doesn't produce the desired precision
            profiler->addDataPoint(
                timer.getElapsedTime().asMicroseconds() * 0.001f);
            profiler->draw(&window);
        }
        window.display();

        sleep:
        sf::sleep(hz - timer.getElapsedTime()); // Only occurs if result > 0
    }
}

void Chip8::onKeyDn(const sf::Event& event)
{
    if (!isPaused)
    {
        const auto key = Keymap.find(event.key.code);
        if (key != Keymap.end())
        {
            vm.setKeyState(key->second, true);
        }
    }
}

void Chip8::onKeyUp(const sf::Event& event)
{
    if (!isPaused)
    {
        const auto key = Keymap.find(event.key.code);
        if (key != Keymap.end())
        {
            vm.setKeyState(key->second, false);
        }
    }

    // Ctrl+P: (un)pause
    if (event.key.control &&
        event.key.code == sf::Keyboard::P)
    {
        isPaused = !isPaused;
    }

    // Ctrl+R: reset VM
    if (event.key.control &&
        event.key.code == sf::Keyboard::R)
    {
        vm.reset();
        isPaused = false;
    }
}

void Chip8::drawFrame()
{
    const auto& frame = vm.frameBuffer();
    sf::VertexArray vertices(sf::Quads, frame.Width * frame.Height * 4);

    for (auto y = 0; y < frame.Height; y++)
    {
        // i = pixel index
        for (auto x = 0, i = y * frame.Width;
             x < frame.Width;
             x++, i = x + y * frame.Width)
        {
            auto px = frame.pixels + i;
            if (*px == 1) // Only process pixels that are 'on'
            {
                auto quad = &vertices[i*4];

                quad[0].position = sf::Vector2f(x*scale,       y*scale);
                quad[1].position = sf::Vector2f(x*scale+scale, y*scale);
                quad[2].position = sf::Vector2f(x*scale+scale, y*scale+scale);
                quad[3].position = sf::Vector2f(x*scale,       y*scale+scale);

                quad[0].color = sf::Color::White;
                quad[1].color = sf::Color::White;
                quad[2].color = sf::Color::White;
                quad[3].color = sf::Color::White;
            }
        }
    }
    window.draw(vertices);
}
