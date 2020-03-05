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
    pen.setBrush(QColor(QString::fromStdString(getString(values, "color", pen.color().name().toStdString()))));
    brush = QBrush(QColor(QString::fromStdString(getString(values, "background", brush.color().name().toStdString()))));
}

int Shape::getInt(map<string, string> &values, const string &key, int defaultValue)
{
    if (values.find(key) != values.end())
        return atoi(values[key].c_str());
    return defaultValue;
}

double Shape::getDouble(map<string, string> &values, const string &key, double defaultValue)
{
    if (values.find(key) != values.end())
        return atof(values[key].c_str());
    return defaultValue;
}

string Shape::getString(map<string, string> &values, const string &key, const string &defaultValue)
{
    if (values.find(key) != values.end())
        return values[key];
    return defaultValue;
}

RectShape::RectShape() : Shape()
{
    x = 0;
    y = 0;
    w = 100;
    h = 100;
}

RectShape::~RectShape()
{
}

void RectShape::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawRect(x, y, w, h);
}

void RectShape::update(map<string, string> &values)
{
    Shape::update(values);
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
    w = getInt(values, "width", w);
    h = getInt(values, "height", h);
}

ArcShape::ArcShape() : Shape()
{
    x = 0;
    y = 0;
    w = 100;
    h = 100;
    start = 0;
    end = 180;
}

ArcShape::~ArcShape()
{
}

void ArcShape::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawArc(x - w / 2, y - w / 2, w, h, start * 16, end * 16);
}

void ArcShape::update(map<string, string> &values)
{
    Shape::update(values);
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
    w = getInt(values, "width", w);
    h = getInt(values, "height", h);
    start = getInt(values, "startAngle", start);
    end = getInt(values, "endAngle", end);
}

TextShape::TextShape() : Shape()
{
    x = 0;
    y = 0;
    size = 16;
    font = "Arial";
    text = "";
}

TextShape::~TextShape()
{
}

void TextShape::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.setFont(QFont(QString::fromStdString(font), size));
    painter.drawText(x, y, QString::fromStdString(text));
}

void TextShape::update(map<string, string> &values)
{
    Shape::update(values);
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
    size = getInt(values, "size", size);
    font = getString(values, "font", font);
    text = getString(values, "text", text);
}

PointShape::PointShape() : Shape()
{
    x = 0;
    y = 0;
}

PointShape::~PointShape()
{
}

void PointShape::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawPoint(x, y);
}

void PointShape::update(map<string, string> &values)
{
    Shape::update(values);
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
}

LineShape::LineShape() : Shape()
{
    x1 = 0;
    y1 = 0;
    x2 = 100;
    y2 = 100;
}

LineShape::~LineShape()
{
}

void LineShape::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawLine(x1, y1, x2, y2);
}

void LineShape::update(map<string, string> &values)
{
    Shape::update(values);
    x1 = getInt(values, "x1", x1);
    y1 = getInt(values, "y1", y1);
    x2 = getInt(values, "x2", x2);
    y2 = getInt(values, "y2", y2);
}

EllipseShape::EllipseShape() : Shape()
{
    x = 0;
    y = 0;
    w = 100;
    h = 100;
}

EllipseShape::~EllipseShape()
{
}

void EllipseShape::draw(QPainter &painter)
{
    Shape::draw(painter);
    painter.drawEllipse(x - w / 2, y - h / 2, w, h);
}

void EllipseShape::update(map<string, string> &values)
{
    Shape::update(values);
    x = getInt(values, "x", x);
    y = getInt(values, "y", y);
    w = getInt(values, "width", w);
    h = getInt(values, "height", h);
}

CircleShape::CircleShape() : EllipseShape()
{
    x = 0;
    y = 0;
    r = 50;
}

CircleShape::~CircleShape()
{
}

void CircleShape::draw(QPainter &painter)
{
    EllipseShape::draw(painter);
}

void CircleShape::update(map<string, string> &values)
{
    EllipseShape::update(values);
    r = getInt(values, "radius", r);
    w = h = r;
}