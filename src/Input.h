#ifndef _INPUT_H_
#define _INPUT_H_

#include <string>

class Input
{
    private:
        bool started;
        size_t pos;
        size_t row, col;
        std::string src;
    public:
        Input();
        ~Input();

        void Fill(const std::string& src);
        bool Eof();
        
        char Peek();
        char Next();

        size_t Col();
        size_t Row();
};

#endif