#include <cmath>
#include <SFML/Graphics.hpp>
#include "chip8.hpp"

#define BG_COL       sf::Color( 41,  43, 49, 255)
#define PX_COL       sf::Color(106, 202, 63, 255)
#define GRAPH_GL_COL sf::Color( 41,  43, 49,  40)
#define GRAPH_OL_COL sf::Color(252,  42, 28, 255)

Chip8::Chip8(unsigned ipc, bool isHighDpi, bool profileCycleTime)
    : ipc(ipc), scale(isHighDpi ? 20 : 10), isPaused(false)
{
    window.create(
        sf::VideoMode(64*scale, 32*scale + (profileCycleTime ? 10*scale : 0)),
        "Chip-8 interpreter");

    if (profileCycleTime)
    {
        auto s = DF::Settings();
        s.caption       = "Cycle time @ " + std::to_string(ipc) + " inst/cycle";
        s.x             = 0;
        s.y             = window.getSize().y - 10*scale;
        s.width         = window.getSize().x;
        s.height        = 10*scale;
        s.bgColor       = sf::Color::White;
        s.borderColor   = sf::Color::White;
        s.gridLines     = 3;
        s.gridLineColor = GRAPH_GL_COL;
        s.lineColor     = BG_COL;
        s.outlierColor  = GRAPH_OL_COL;
        s.fontColor     = BG_COL;
        s.fontName      = "font.ttf";
        s.vscale        = 16;
        s.vscaleUnit    = " ms";
        s.showMean      = true;

        profiler = std::unique_ptr<DF>(new DF(s));
    }

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
        samples[i] = amplitude * std::sin(angularFreq * (interval * i));
    }

    buzzerBuffer.loadFromSamples(&samples[0], sampleRate, 1, sampleRate);
    buzzer.setBuffer(buzzerBuffer);
    buzzer.setLoop(true);
}

void Chip8::run(const std::string& rom)
{
    if (!vm.loadProgram(rom)) return;
    window.setTitle(vm.programInfo().name);

    float delta;
    const auto hz = sf::milliseconds(1000/60); // 60 Hz, 16.6 ms
    sf::Clock timer;
    for (;;)
    {
        delta = 0.0f;
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
        delta = timer.getElapsedTime().asMicroseconds() * 0.001f;

        window.clear(BG_COL);
        drawFrame();
        if (profiler)
        {
            // Log how long this cycle took (in ms)
            profiler->addDataPoint(delta);
            window.draw(*profiler);
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
        buzzer.stop();
        isPaused = !isPaused;
        window.setTitle(isPaused ? "**PAUSED**"
                                 : vm.programInfo().name);
    }

    // Ctrl+R: reset VM
    if (event.key.control &&
        event.key.code == sf::Keyboard::R)
    {
        buzzer.stop();
        isPaused = false;
        window.setTitle(vm.programInfo().name);
        if (profiler) profiler->clear();
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
                auto quad = &vertices[i*4];

                quad[0].position = sf::Vector2f(x*scale,       y*scale);
                quad[1].position = sf::Vector2f(x*scale+scale, y*scale);
                quad[2].position = sf::Vector2f(x*scale+scale, y*scale+scale);
                quad[3].position = sf::Vector2f(x*scale,       y*scale+scale);

                quad[0].color = PX_COL;
                quad[1].color = PX_COL;
                quad[2].color = PX_COL;
                quad[3].color = PX_COL;
            }
        }
    }
    window.draw(vertices);
}
