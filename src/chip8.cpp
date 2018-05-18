#include <cmath>
#include "chip8.hpp"

#define BG_COL       sf::Color( 41,  43, 49, 255)
#define PX_COL       sf::Color(106, 202, 63, 255)
#define GRAPH_GL_COL sf::Color( 41,  43, 49,  40)
#define GRAPH_OL_COL sf::Color(252,  42, 28, 255)

Chip8::Chip8(unsigned ipc, bool isHighDpi)
    : ipc(ipc), scale(isHighDpi ? 20U : 10U), isPaused(false)
{
    window.create(sf::VideoMode(64U * scale, 32U * scale), "Chip-8 interpreter");
    initSound();
}

// Create 1.4 kHz tone for buzzer sound
void Chip8::initSound()
{
    constexpr auto amplitude   = 5000;
    constexpr auto freq        = 1400; // Hz
    constexpr auto sampleRate  = 8000;
    constexpr auto interval    = 1.0f/sampleRate;
    constexpr auto angularFreq = 6.28318f * freq; // 2πf (w)

    sf::Int16 samples[sampleRate];
    for (auto i = 0; i < sampleRate; i++)
    {
        // y(t) = A.sin(w.t)
        samples[i] = sf::Int16(amplitude * std::sin(angularFreq * (interval * i)));
    }

    buzzerBuffer.loadFromSamples(&samples[0], sampleRate, 1, sampleRate);
    buzzer.setBuffer(buzzerBuffer);
    buzzer.setLoop(true);
}

void Chip8::run(const std::string& rom, bool withCompatibility)
{
    if (!vm.loadProgram(rom)) return;

    window.setTitle(vm.programInfo().name);
    vm.useAltShiftLoadBehaviour(withCompatibility);

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
        for (auto i = 0U; i < ipc; i++)
        {
            vm.cycle();
            vm.isBuzzerOn() ? buzzer.play() : buzzer.stop();
        }

        vm.cycleTimers(); // Update timers at 60 Hz independent of IPC

        window.clear(BG_COL);
        drawFrame();
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
        buzzer.stop();
        isPaused = !isPaused;
        window.setTitle(isPaused ? "**PAUSED**" : vm.programInfo().name);
    }

    // Ctrl+R: reset VM
    if (event.key.control &&
        event.key.code == sf::Keyboard::R)
    {
        buzzer.stop();
        isPaused = false;
        window.setTitle(vm.programInfo().name);
        vm.reset();
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
                auto quad = &vertices[i * 4];

                quad[0].position = sf::Vector2f(float(x*scale),       float(y*scale));
                quad[1].position = sf::Vector2f(float(x*scale+scale), float(y*scale));
                quad[2].position = sf::Vector2f(float(x*scale+scale), float(y*scale+scale));
                quad[3].position = sf::Vector2f(float(x*scale),       float(y*scale+scale));

                quad[0].color = PX_COL;
                quad[1].color = PX_COL;
                quad[2].color = PX_COL;
                quad[3].color = PX_COL;
            }
        }
    }
    window.draw(vertices);
}
