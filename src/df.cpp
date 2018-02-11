#include <iomanip>
#include <sstream>
#include "df.hpp"

DF::DF(const Settings settings)
{
    // The number of data points displayed will always be <= the graph width
    graphData.reserve(settings.width);

    applySettings(settings);
    clear();
}

void DF::clear()
{
    yPoints.clear();
    graphData.clear();
    mean = max = 0;
    n = 0;
}

void DF::applySettings(const Settings settings)
{
    s = settings;
    createGraph();
}

DF::Settings DF::settings() const
{
    return s;
}

void DF::addDataPoint(float dp)
{
    max  = std::max(max, dp);
    mean = (mean * n + dp) / (n + 1);
    n++;

    statsText.setString(s.title + " (samples: " + std::to_string(n)
        + ", mean: " + toDecimalString(mean, 2) + s.vscaleUnit
        + ", max: "  + toDecimalString(max,  2) + s.vscaleUnit + ")");

    yPoints.push_front(s.y + s.height - dp * (s.height / float(s.vscale)));
    if (yPoints.size() > s.width)
    {
        // Ensure the size does not exceed the width of the graph
        yPoints.pop_back();
    }

    // Update the vertex array
    auto x = s.x + s.width;
    for (std::size_t i = 0; i < yPoints.size(); i++, x--)
    {
        // Insert a new vertex if needed
        if (i >= graphData.size())
        {
            graphData.push_back(sf::Vertex());
        }

        auto& v = graphData[i];
        v.color      = s.lineColor;
        v.position.x = x;
        v.position.y = yPoints[i];

        // Clamp points that overflow
        if (v.position.y < s.y)
        {
            v.position.y = s.y;
            v.color = s.overColor;
        }
    }
}

void DF::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(border);
    target.draw(axisLine);
    target.draw(&graphData[0], graphData.size(), sf::LinesStrip);
    target.draw(axisText1);
    target.draw(axisText2);
    target.draw(statsText);
}

void DF::createGraph()
{
    border.setPosition(s.x, s.y);
    border.setSize(sf::Vector2f(s.width, s.height));
    border.setOutlineThickness(1);
    border.setOutlineColor(sf::Color::White);
    border.setFillColor(sf::Color(0, 0, 0,
        std::min(1.0f, std::max(0.0f, s.transparency)) * 255));

    axisLine.resize(2);
    axisLine.clear();
    axisLine.setPrimitiveType(sf::LineStrip);
    axisLine.append(
        sf::Vertex(sf::Vector2f(
            s.x,
            s.y + s.height * 0.5f), sf::Color::White));
    axisLine.append(
        sf::Vertex(sf::Vector2f(
            s.x + s.width,
            s.y + s.height * 0.5f), sf::Color::White));

    if (font.loadFromFile(s.fontName))
    {
        statsText.setFont(font);
        statsText.setFillColor(sf::Color::White);
        statsText.setCharacterSize(s.height * 0.125f);
        statsText.setPosition(sf::Vector2f(s.x + 8, s.y));

        axisText1.setFont(font);
        axisText1.setFillColor(sf::Color::White);
        axisText1.setCharacterSize(statsText.getCharacterSize());
        axisText1.setString(toDecimalString(s.vscale, 0) + s.vscaleUnit);
        axisText1.setPosition(sf::Vector2f(
            s.x + s.width - axisText1.getLocalBounds().width - 12,
            s.y));

        axisText2.setFont(font);
        axisText2.setFillColor(sf::Color::White);
        axisText2.setCharacterSize(statsText.getCharacterSize());
        axisText2.setString(toDecimalString(s.vscale * 0.5f, 0) + s.vscaleUnit);
        axisText2.setPosition(sf::Vector2f(
            s.x + s.width - axisText2.getLocalBounds().width - 12,
            s.y + s.height * 0.5f));
    }
}

std::string DF::toDecimalString(float f, char precision)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << f;
    return ss.str();
}
