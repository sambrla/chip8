#ifndef CHIP8_DF_H_
#define CHIP8_DF_H_

#include <deque>
#include <SFML/Graphics/RenderWindow.hpp>

// Quick and dirty class to display diagnostic data. Requires SFML
class DF
{
public:
    struct Settings
    {
        unsigned  x         = 0;
        unsigned  y         = 0;
        unsigned  height    = 0;
        unsigned  width     = 0;
        unsigned  vscale    = 1;
        sf::Color lineColor = sf::Color::White;

        // Corresponding font should be located in project dir
        // Designed for fixed width fonts
        std::string fontName;
        std::string vscaleLabel;

        // Background transparency; between 0 (transparent) and 1 (opaque)
        float transparency = 0;
    };

    explicit DF(Settings settings);

    Settings settings() const;
    void applySettings(Settings s);
    void addDataPoint(float dp);
    void drawGraph(sf::RenderWindow* win);
    void clear();

private:
    Settings s;
    std::deque<sf::Vertex> points;
    float average = 0;
    unsigned n = 0;

    // Graph components
    sf::RectangleShape graphBorder;
    sf::VertexArray graphLine;
    sf::Text stat, axis1, axis2;
    sf::Font font;

    void createGraph();
    static std::string toDecimalString(float f, char precision);
};

#endif // CHIP8_DF_H_
