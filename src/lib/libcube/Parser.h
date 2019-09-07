#ifndef _PARSER_H_
#define _PARSER_H_

#include "Tokenizer.h"
#include "Node.h"
#include <map>
#include <set>

class Parser
{
    private:
        Tokenizer tokens;
        std::map<std::string, int> precedence;
        std::set<std::string> noright, noleft;

        void MakeError(Node &node, const std::string& msg, Token &token);
        
        Node ParseExpression();

        Node MaybeBinary(Node left, int prec);
        bool DelimitedNodes(std::vector<Node> &nodes, const std::string &start, const std::string &stop, const std::string &separator, bool op = false);
        bool DelimitedNames(std::vector<std::string> &names, const std::string &start, const std::string &stop, const std::string &separator, bool op = false);
        Node ParseCall(Node func);
        std::string ParseVarName();
        std::string ParseFuncName();
        std::string ParseVarNameOrString();
        Node ParseIf();
        Node ParseLet();
        Node ParseLambda();
        Node ParseFunction();
        Node ParseSpawn();
        Node ParseFor();
        Node ParseWhile();
        Node ParseDoWhile();
        Node ParseArray();
        Node ParseBool();
        Node MaybeCall(Node expr);
        Node ParseAtom();
        Node ParseContext();
        Node False();
        Node Ignore();
        Node ParseImport();
        Node ParseTry();
        Node ParseClass();

        bool HasOperator(const std::string& op);
        bool IsSymbol(const std::string& symbol = "");
        bool IsKeyword(const std::string& keyword = "");
        bool IsOperator(const std::string& op = "");
        bool SkipSymbol(const std::string& symbol);
        bool SkipKeyword(const std::string& keyword);
        bool SkipOperator(const std::string& op);
    public:
        Parser();
        ~Parser();

        Node Parse(const std::string& src);
};

#endif
