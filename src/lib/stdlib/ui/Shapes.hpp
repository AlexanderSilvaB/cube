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
        std::string getString(std::map<std::string, std::string> &values, const std::string &key, const std::string &defaultValue);
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

class Rect : public Shape
{
    public:
        int x, y, w, h;

        Rect();
        ~Rect();
        void draw(QPainter &painter);
        void update(std::map<std::string, std::string> &values);
};

class Arc : public Shape
{
    public:
        int x, y, w, h;
        int start, end;

        Arc();
        ~Arc();
        void draw(QPainter &painter);
        void update(std::map<std::string, std::string> &values);
};

class Text : public Shape
{
    public:
        int x, y;
        int size;
        std::string text;

        Text();
        ~Text();
        void draw(QPainter &painter);
        void update(std::map<std::string, std::string> &values);
};

class Point : public Shape
{
    public:
        int x, y;

        Point();
        ~Point();
        void draw(QPainter &painter);
        void update(std::map<std::string, std::string> &values);
};

class Line : public Shape
{
    public:
        int x1, y1, x2, y2;

        Line();
        ~Line();
        void draw(QPainter &painter);
        void update(std::map<std::string, std::string> &values);
};

#endif
