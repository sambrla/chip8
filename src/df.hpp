#ifndef DF_H_
#define DF_H_

#include <deque>
#include <SFML/Graphics.hpp>

// Quick and dirty class to display diagnostic data. Requires SFML
class DF
{
public:
    struct Settings
    {
        unsigned  x      = 0;
        unsigned  y      = 0;
        unsigned  height = 0;
        unsigned  width  = 0;
        unsigned  vscale = 0;

        sf::Color lineColor = sf::Color::White;
        sf::Color overColor = sf::Color::Red;

        // Corresponding font should be located in project dir
        // Designed for fixed width fonts
        std::string fontName;
        std::string vscaleLabel;

        // Background transparency; between 0 (transparent) and 1 (opaque)
        float transparency = 1;
    };

    explicit DF(const Settings settings);
    Settings settings() const;
    void applySettings(const Settings settings);
    void addDataPoint(float dp);
    void draw(sf::RenderWindow* win);
    void clear();

private:
    Settings s;
    std::deque<sf::Vertex> points;
    float mean, max;
    unsigned n;

    // Graph components
    sf::RectangleShape graphBorder;
    sf::VertexArray graphLine;
    sf::Text stats, axis1, axis2;
    sf::Font font;

    void createGraph();
    static std::string toDecimalString(float f, char precision);
};

#endif // DF_H_
