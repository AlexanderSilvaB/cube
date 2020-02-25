#include "Shapes.hpp"

using namespace std;

Shape::Shape()
{
    id = 0;
    angle = 0;
    scale = 1;
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(3);
    pen.setColor(Qt::green);
    pen.setCapStyle(Qt::SquareCap);
    pen.setJoinStyle(Qt::RoundJoin);
}

Shape::~Shape()
{

}

void Shape::set(QPainter &painter)
{
    painter.setPen(pen);
    painter.setBrush(brush);
}

void Shape::draw(QPainter &painter)
{
    painter.rotate(angle);
    painter.scale(scale, scale);
}

void Shape::update(map<string, string> &values)
{
    angle = getDouble(values, "angle", angle);
    scale = getDouble(values, "scale", scale);
    pen.setBrush( QColor( QString::fromStdString(getString(values, "color", pen.color().name().toStdString() )) ) );
    brush = QBrush( QColor( QString::fromStdString(getString(values, "background", brush.color().name().toStdString() )) ) );
}

int Shape::getInt(map<string, string> &values, const string &key, int defaultValue)
{
    if(values.find(key) != values.end())
        return atoi(values[key].c_str());
    return defaultValue;
}

double Shape::getDouble(map<string, string> &values, const string &key, double defaultValue)
{
    if(values.find(key) != values.end())
        return atof(values[key].c_str());
    return defaultValue;
}

string Shape::getString(map<string, string> &values, const string &key, const string &defaultValue)
{
    if(values.find(key) != values.end())
        return values[key];
    return defaultValue;
}

Rect::Rect() : Shape()
{
    x = 0;
    y = 0;
    w = 100;
    h = 100;
}

Rect::~Rect()
{

}

void Rect::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawRect(x, y, w, h);
}

void Rect::update(map<string, string> &values)
{
    Shape::update(values);
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
    w = getInt(values, "width", w);
    h = getInt(values, "height", h);
}

Arc::Arc() : Shape()
{
    x = 0;
    y = 0;
    w = 100;
    h = 100;
    start = 0;
    end = 180;
}

Arc::~Arc()
{

}

void Arc::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawArc(x, y, w, h, start * 16, end * 16);
}

void Arc::update(map<string, string> &values)
{
    Shape::update(values);
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
    w = getInt(values, "width", w);
    h = getInt(values, "height", h);
    start = getInt(values, "startAngle", start);
    end = getInt(values, "endAngle", end);
}

Text::Text() : Shape()
{
    x = 0;
    y = 0;
    size = 16;
    text = "";
}

Text::~Text()
{

}

void Text::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawText(x, y, QString::fromStdString(text));
}

void Text::update(map<string, string> &values)
{
    Shape::update(values);
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
    size = getInt(values, "size", size);
    text = getString(values, "text", text);
}

Point::Point() : Shape()
{
    x = 0;
    y = 0;
}

Point::~Point()
{

}

void Point::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawPoint(x, y);
}

void Point::update(map<string, string> &values)
{
    Shape::update(values);
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
}

Line::Line() : Shape()
{
    x1 = 0;
    y1 = 0;
    x2 = 100;
    y2 = 100;
}

Line::~Line()
{

}

void Line::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawLine(x1, y1, x2, y2);
}

void Line::update(map<string, string> &values)
{
    Shape::update(values);
    x1 = getInt(values, "x1", x1);
    y1 = getInt(values, "y1", y1);
    x2 = getInt(values, "x2", x2);
    y2 = getInt(values, "y2", y2);
}