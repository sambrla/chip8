#include <iomanip>
#include <sstream>
#include "df.hpp"

DF::DF()
{
}

DF::DF(DF::Settings settings)
{
    applySettings(settings);
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
    average = (average * n + dp) / (n + 1);
    n++;

    // x is set to 0 as it will be overwritten
    sf::Vertex v(sf::Vector2f(
        0,
        s.y + s.height - dp * (s.height / float(s.vscale))), s.lineColor);
    points.push_front(v);

    // The collection should not exceed the width of the graph area
    if (points.size() > s.width)
        points.pop_back();
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
            p.color = sf::Color::Red;
        }

        data.append(p);
    }

    win->draw(data);
    win->draw(axis1);
    win->draw(axis2);

    stat.setString(std::to_string(n) + " samples, mean: "
        + toDecimalString(average, 2) + s.vscaleLabel);
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
    graphBorder.setPosition(s.x, s.y);
    graphBorder.setSize(sf::Vector2f(s.width, s.height));
    graphBorder.setOutlineThickness(1);
    graphBorder.setOutlineColor(sf::Color::White);
    graphBorder.setFillColor(sf::Color(0, 0, 0,
        std::min(1.0f, std::max(0.0f, s.transparency)) * 255));

    graphLine.resize(2);
    graphLine.clear();
    graphLine.setPrimitiveType(sf::LineStrip);
    graphLine.append(
        sf::Vertex(sf::Vector2f(
            s.x,
            s.y + s.height * 0.5f), sf::Color::White));
    graphLine.append(
        sf::Vertex(sf::Vector2f(
            s.x + s.width,
            s.y + s.height * 0.5f), sf::Color::White));

    if (font.loadFromFile(s.fontName))
    {
        stat.setFont(font);
        stat.setFillColor(sf::Color::White);
        stat.setCharacterSize(s.height * 0.125f);
        stat.setPosition(sf::Vector2f(s.x + 8, s.y));

        axis1.setFont(font);
        axis1.setFillColor(sf::Color::White);
        axis1.setCharacterSize(stat.getCharacterSize());
        axis1.setString(toDecimalString(s.vscale, 0) + s.vscaleLabel);
        axis1.setPosition(sf::Vector2f(
            s.x + s.width - axis1.getLocalBounds().width - 12,
            s.y));

        axis2.setFont(font);
        axis2.setFillColor(sf::Color::White);
        axis2.setCharacterSize(stat.getCharacterSize());
        axis2.setString(toDecimalString(s.vscale * 0.5f, 0) + s.vscaleLabel);
        axis2.setPosition(sf::Vector2f(
            s.x + s.width - axis2.getLocalBounds().width - 12,
            s.y + s.height * 0.5f));
    }
}

std::string DF::toDecimalString(float f, char precision)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << f;
    return ss.str();
}
