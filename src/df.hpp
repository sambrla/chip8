#ifndef DF_H_
#define DF_H_

#include <deque>
#include <SFML/Graphics.hpp>

// Plots diagnostic data. Requires SFML
class DF : public sf::Drawable
{
public:
    struct Settings
    {
        int x         = 0;
        int y         = 0;
        int height    = 0;
        int width     = 0;
        int vscale    = 0;
        int gridLines = 1;

        sf::Color lineColor     = sf::Color::Cyan;
        sf::Color outlierColor  = sf::Color::Blue;
        sf::Color bgColor       = sf::Color::Black;
        sf::Color gridLineColor = sf::Color::White;
        sf::Color borderColor   = sf::Color::White;
        sf::Color fontColor     = sf::Color::White;

        // Bold font face is optional; will fall back to regular if not found
        std::string fontNameRegular;
        std::string fontNameBold;
        std::string vscaleUnit;
        std::string title;
        std::string caption;
    };

    explicit DF(const Settings settings);
    Settings settings() const;
    float average() const;
    void addDataPoint(float dp);
    void applySettings(const Settings settings);
    void clear();
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    Settings s;
    std::deque<int> yPointsBuffer;
    std::vector<sf::Vertex> xyPoints;
    int n;
    float avg;

    // Graph components
    sf::RectangleShape border;
    sf::VertexArray gridLines;
    sf::Text titleText, captionText, scaleText;
    sf::Font fontRegular, fontBold;

    void createGraph();
};

#endif // DF_H_
