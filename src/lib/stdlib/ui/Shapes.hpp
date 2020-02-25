#ifndef _CUBE_SHAPES_HPP_
#define _CUBE_SHAPES_HPP_

#include <QtWidgets>
#include <map>
#include <string>

class Shape
{
    protected:
        int getInt(std::map<std::string, std::string> &values, const std::string &key, int defaultValue);
        std::string getString(std::map<std::string, std::string> &values, const std::string &key, const std::string &defaultValue);
    public:
        int id;
        QPen pen;
        QBrush brush;

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

#endif
