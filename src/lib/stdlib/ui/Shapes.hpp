#ifndef _CUBE_SHAPES_HPP_
#define _CUBE_SHAPES_HPP_

#include <QtWidgets>
#include <map>
#include <string>

class Shape
{
  protected:
    int getInt(std::map<std::string, std::string> &values, const std::string &key, int defaultValue);
    double getDouble(std::map<std::string, std::string> &values, const std::string &key, double defaultValue);
    std::string getString(std::map<std::string, std::string> &values, const std::string &key,
                          const std::string &defaultValue);

  public:
    int id;
    QPen pen;
    QBrush brush;
    double angle, scale;

    Shape();
    virtual ~Shape();

    void set(QPainter &painter);
    virtual void draw(QPainter &painter);
    virtual void update(std::map<std::string, std::string> &values);
};

class RectShape : public Shape
{
  public:
    int x, y, w, h;

    RectShape();
    ~RectShape();
    void draw(QPainter &painter);
    void update(std::map<std::string, std::string> &values);
};

class ArcShape : public Shape
{
  public:
    int x, y, w, h;
    int start, end;

    ArcShape();
    ~ArcShape();
    void draw(QPainter &painter);
    void update(std::map<std::string, std::string> &values);
};

class TextShape : public Shape
{
  public:
    int x, y;
    int size;
    std::string font;
    std::string text;

    TextShape();
    ~TextShape();
    void draw(QPainter &painter);
    void update(std::map<std::string, std::string> &values);
};

class PointShape : public Shape
{
  public:
    int x, y;

    PointShape();
    ~PointShape();
    void draw(QPainter &painter);
    void update(std::map<std::string, std::string> &values);
};

class LineShape : public Shape
{
  public:
    int x1, y1, x2, y2;

    LineShape();
    ~LineShape();
    void draw(QPainter &painter);
    void update(std::map<std::string, std::string> &values);
};

class EllipseShape : public Shape
{
  public:
    int x, y, w, h;

    EllipseShape();
    ~EllipseShape();
    void draw(QPainter &painter);
    void update(std::map<std::string, std::string> &values);
};

class CircleShape : public EllipseShape
{
  public:
    int r;

    CircleShape();
    ~CircleShape();
    void draw(QPainter &painter);
    void update(std::map<std::string, std::string> &values);
};

class ImageShape : public Shape
{
  public:
    int x, y, w, h;
    int sx, sy, sw, sh;
    QImage image;

    ImageShape();
    ~ImageShape();
    void draw(QPainter &painter);
    void update(std::map<std::string, std::string> &values);
};

#endif
