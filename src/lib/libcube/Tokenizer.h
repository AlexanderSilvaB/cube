#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include "Input.h"
#include "Token.h"
#include <list>
#include <set>

class Tokenizer
{
    private:
        Input input;
        Token token;
        bool eof;
        std::list<std::string> keywords;
        std::set<std::string> operators;

        static bool IsDigit(char c);
        static bool IsLetter(char c);
        static bool IsOperator(char c);
        static bool IsSymbol(char c);

        char SkipWhite();
        char SkipComment();
        void ReadString();
        void ReadNumber();
        void ReadName();
        void ReadOperator();

        bool IsOperator(const std::string& op);

        void MakeError(const std::string& msg, char c = 0);
    public:
        Tokenizer();
        ~Tokenizer();

        void Fill(const std::string& src);
        bool Eof();

        Token& Peek();
        Token& Next();
};

#endif
