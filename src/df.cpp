#include <iomanip>
#include <sstream>
#include <SFML/Graphics.hpp>
#include "df.hpp"

DF::DF(DF::Settings settings)
{
    applySettings(settings);
}

DF::Settings DF::settings() const
{
    return s;
}

void DF::applySettings(Settings settings)
{
    s = settings;
    createGraph();
}

void DF::addDataPoint(float dp)
{
    average = (average * n + dp) / (n + 1);
    n++;

    sf::Vertex v(sf::Vector2f(
        0, // Will be overwritten
        s.y + s.height - dp * (s.height / s.scale)), s.lineColor);
    points.push_front(v);

    // The collection should not exceed the width of the graph area
    if (points.size() > s.width) points.pop_back();
}

void DF::drawGraph(sf::RenderWindow* win)
{
    win->draw(graphBorder);
    win->draw(graphLine);     

    sf::VertexArray data(sf::LineStrip, points.size());
    data.clear();

    auto x = s.x + s.width;
    for (auto p : points)
    {
        // Prevent points appearing outside left margin
        if (x <= s.x) break;

        p.position.x = --x;
        if (p.position.y < s.y)
        {
            // Clamp points that fall outside drawGraph scale
            p.position.y = s.y;

            if (s.highlightOverflow)
            {
                overflow.setSize(sf::Vector2f(8, s.height));
                overflow.setPosition(p.position.x - 4, s.y);
                overflow.setFillColor(s.overflowColor);
                win->draw(overflow);
            }
        }
        
        data.append(p);
    }

    win->draw(data);
    win->draw(axis1);
    win->draw(axis2);

    stat.setString("Avg " + toDecimalString(average, 1) + s.scaleUnit
        + " (" + std::to_string(n) + " samples)");
    win->draw(stat);
}

void DF::clear()
{
    points.clear();
    average = 0;
    n = 0;
}

void DF::createGraph()
{
    graphBorder.setSize(sf::Vector2f(s.width, s.height));
    graphBorder.setPosition(s.x, s.y);
    graphBorder.setOutlineColor(sf::Color::White);
    graphBorder.setOutlineThickness(1);
    graphBorder.setFillColor(sf::Color(0, 0, 0,
        std::min(1.0f, std::max(0.0f, s.transparency)) * 255));

    graphLine.resize(2);
    graphLine.clear();
    graphLine.setPrimitiveType(sf::LineStrip);
    graphLine.append(
        sf::Vertex(sf::Vector2f(
            s.x,
            s.y + s.height / 2), sf::Color::White));
    graphLine.append(
        sf::Vertex(sf::Vector2f(
            s.x + s.width,
            s.y + s.height / 2), sf::Color::White));

    if (font.loadFromFile(s.fontName))
    {
        stat.setFont(font);
        stat.setFillColor(sf::Color::White);
        stat.setCharacterSize(11);
        stat.setPosition(sf::Vector2f(s.x + 4, s.y));

        axis1.setFont(font);
        axis1.setFillColor(sf::Color::White);
        axis1.setCharacterSize(11);
        axis1.setString(toDecimalString(s.scale, 0) + s.scaleUnit);
        axis1.setPosition(sf::Vector2f(
            s.x + s.width - axis1.getLocalBounds().width - 8,
            s.y));

        axis2.setFont(font);
        axis2.setFillColor(sf::Color::White);
        axis2.setCharacterSize(11);
        axis2.setString(toDecimalString(s.scale / 2, 0) + s.scaleUnit);
        axis2.setPosition(sf::Vector2f(
            s.x + s.width - axis1.getLocalBounds().width - 8,
            s.y + s.height / 2));
    }
}

std::string DF::toDecimalString(float f, char precision)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << f;
    return ss.str();
}
