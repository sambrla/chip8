#include "df.hpp"

DF::DF(const Settings settings)
{
    // The number of data points displayed will be <= the graph width
    xyPoints.reserve(settings.width);

    clear();
    applySettings(settings);
}

void DF::clear()
{
    yPointsBuffer.clear();
    xyPoints.clear();
    avg = n = 0;
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

float DF::average() const
{
    return avg;
}

void DF::addDataPoint(float dp)
{
    avg = (avg * n + dp) / (n + 1);
    n++;

    yPointsBuffer.push_front(s.y + s.height - dp * (s.height / float(s.vscale)));
    if (yPointsBuffer.size() > s.width)
    {
        // Ensure the size does not exceed the width of the graph
        yPointsBuffer.pop_back();
    }

    // Update the vertex array
    auto x = s.x + s.width;
    for (std::size_t i = 0; i < yPointsBuffer.size(); i++, x--)
    {
        if (i >= xyPoints.size())
        {
            // Insert a new vertex if needed
            xyPoints.push_back(sf::Vertex());
        }

        auto& v = xyPoints[i];
        v.color      = s.lineColor;
        v.position.x = x;
        v.position.y = yPointsBuffer[i];

        // Clamp points that overflow
        if (v.position.y < s.y)
        {
            v.position.y = s.y;
            v.color = s.outlierColor;
        }
    }
}

void DF::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(border);
    target.draw(gridLines);
    target.draw(scaleText);
    target.draw(titleText);
    target.draw(captionText);
    target.draw(&xyPoints[0], xyPoints.size(), sf::LineStrip);
}

void DF::createGraph()
{
    border.setPosition(s.x, s.y);
    border.setSize(sf::Vector2f(s.width, s.height));
    border.setOutlineThickness(1);
    border.setOutlineColor(s.borderColor);
    border.setFillColor(s.bgColor);

    gridLines.resize(s.gridLines * 2);
    gridLines.clear();
    gridLines.setPrimitiveType(sf::Lines);
    for (auto i = 1, d = s.height / (s.gridLines+1); i <= s.gridLines; i++)
    {
        gridLines.append(sf::Vertex(
            sf::Vector2f(s.x,
                         s.y + s.height - i*d), s.gridLineColor));

        gridLines.append(sf::Vertex(
            sf::Vector2f(s.x + s.width,
                         s.y + s.height - i*d), s.gridLineColor));
    }

    if (fontRegular.loadFromFile(s.fontNameRegular))
    {
        const auto xPadding = s.width  * 0.01f;
        const auto yPadding = s.height * 0.05f;
        const auto h1 = s.height * 0.12f;
        const auto h2 = s.height * 0.10f;

        titleText.setFont(
            fontBold.loadFromFile(s.fontNameBold) ? fontBold : fontRegular);
        titleText.setFillColor(s.fontColor);
        titleText.setCharacterSize(h1); // Scale font to height
        titleText.setString(s.title);
        titleText.setPosition(sf::Vector2f(
            s.x + xPadding,
            s.y + yPadding));

        captionText.setFont(fontRegular);
        captionText.setFillColor(s.fontColor);
        captionText.setCharacterSize(h2);
        captionText.setString(s.caption);
        captionText.setPosition(sf::Vector2f(
            s.x + xPadding,
            s.y + h1 + (yPadding*2)));

        scaleText.setFont(fontRegular);
        scaleText.setFillColor(s.fontColor);
        scaleText.setCharacterSize(h2);
        scaleText.setString("scale: " + std::to_string(s.vscale) + s.vscaleUnit);
        scaleText.setPosition(sf::Vector2f(
            s.x + s.width - scaleText.getLocalBounds().width - xPadding,
            s.y + yPadding));
    }
}
