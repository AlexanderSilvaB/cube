#include "Shapes.hpp"

using namespace std;

Shape::Shape()
{
    id = 0;
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

}

void Shape::update(map<string, string> &values)
{

}

int Shape::getInt(map<string, string> &values, const string &key, int defaultValue)
{
    if(values.find(key) != values.end())
        return atoi(values[key].c_str());
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
    painter.drawRect(x, y, w, h);
}

void Rect::update(map<string, string> &values)
{
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
    w = getInt(values, "width", w);
    h = getInt(values, "height", h);
    pen.setBrush( QColor( QString::fromStdString(getString(values, "color", pen.color().name().toStdString() )) ) );
    brush = QBrush( QColor( QString::fromStdString(getString(values, "background", brush.color().name().toStdString() )) ) );
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
    painter.drawArc(x, y, w, h, start * 16, end * 16);
}

void Arc::update(map<string, string> &values)
{
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
    w = getInt(values, "width", w);
    h = getInt(values, "height", h);
    start = getInt(values, "startAngle", start);
    end = getInt(values, "endAngle", end);
    pen.setBrush( QColor( QString::fromStdString(getString(values, "color", pen.color().name().toStdString() )) ) );
    brush = QBrush( QColor( QString::fromStdString(getString(values, "background", brush.color().name().toStdString() )) ) );
}