#ifndef DF_H_
#define DF_H_

#include <deque>
#include <SFML/Graphics.hpp>

// Quick and dirty class to display diagnostic data. Requires SFML
class DF : public sf::Drawable
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
        std::string vscaleUnit;
        std::string title;

        // Background transparency; between 0 (transparent) and 1 (opaque)
        float transparency = 1;
    };

    explicit DF(const Settings settings);
    Settings settings() const;
    void applySettings(const Settings settings);
    void addDataPoint(float dp);
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    void clear();

private:
    Settings s;
    std::deque<unsigned> yPoints;
    std::vector<sf::Vertex> graphData;
    float mean, max;
    unsigned n;

    // Graph components
    sf::RectangleShape border;
    sf::VertexArray axisLine;
    sf::Text statsText, axisText1, axisText2;
    sf::Font font;

    void createGraph();
    static std::string toDecimalString(float f, char precision);
};

#endif // DF_H_
