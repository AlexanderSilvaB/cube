#include "Parser.h"
#include <iostream>

using namespace std;

Parser::Parser()
{
    precedence["="] = 1;//ok
    precedence["."] = 2;//ok
    precedence["in"] = 3;
    precedence["++"] = 3;//ok
    precedence["--"] = 3;//ok
    precedence["||"] = 4;//ok
    precedence["&&"] = 5;//ok
    precedence["<"] = 8;//ok
    precedence[">"] = 8;//ok
    precedence["<="] = 8;//ok
    precedence[">="] = 8;//ok
    precedence["=="] = 8;//ok
    precedence["!="] = 8;//ok
    precedence["|"] = 8;//ok
    precedence["&"] = 8;//ok
    precedence["<<"] = 8;//ok
    precedence[">>"] = 8;//ok
    precedence["~"] = 8;//ok
    precedence["^"] = 8;//ok
    precedence["!"] = 9;//ok
    precedence["+"] = 10;//ok
    precedence["-"] = 10;//ok
    precedence["*"] = 20;//ok
    precedence["/"] = 20;//ok
    precedence["%"] = 20;//ok
    precedence["**"] = 25;//ok
    precedence[":"] = 30;//ok

    noright.insert("++");
    noright.insert("--");
    noright.insert("!");

    noleft.insert("+");
    noleft.insert("-");
    noleft.insert("++");
    noleft.insert("--");
    noleft.insert("~");
}

Parser::~Parser()
{

}

Node Parser::Parse(const string& src)
{
    Node root = MKNODE();
    root->type = NodeType::CONTEXT;

    tokens.Fill(src);

    tokens.Next();
    while(!tokens.Eof())
    {
        Node node = ParseExpression();
        if(node->type == NodeType::ERROR)
        {
            return node;
        }
        if(!tokens.Eof())
        {
            SkipSymbol(";");
        }
        root->nodes.push_back(node);
    }

    return root;
}

Node Parser::ParseExpression()
{
    Node atom = ParseAtom();
    if(atom->type == NodeType::ERROR)
        return atom;
    Node bin = MaybeBinary(atom, 0);
    if(bin->type == NodeType::ERROR)
        return bin;
    return MaybeCall(bin);
}

Node Parser::MaybeBinary(Node left, int prec)
{
    if(IsOperator() || IsKeyword("in"))
    {
        Token tok = tokens.Peek();
        if(tok._string == "[")
        {
            Node node = MKNODE();
            node->_string = "[]";
            node->type = NodeType::INDEX;
            node->left = left;
            if(!DelimitedNodes(node->nodes, "[", "]", ",", true))
            {
                MakeError(node, "Invalid array index", tokens.Peek());
                return node;
            }
            return MaybeBinary(node, prec);
        }
        else
        {
            int nprec = 0;
            if(HasOperator(tok._string))
            {
                nprec = precedence[tok._string];
            }
            if(nprec > prec)
            {
                tokens.Next();
                Node node = MKNODE();
                node->_string = tok._string;
                node->type = tok._string == "=" ? NodeType::ASSIGN : NodeType::BINARY;
                node->left = left;

                if(noright.count(tok._string) == 0 || (left->type == NodeType::IGNORE && noleft.count(tok._string) > 0))
                {
                    Node right = ParseAtom();
                    if(right->type == NodeType::ERROR)
                        return right;
                    node->right = MaybeBinary(right, nprec);
                    if(!node->middle && !node->left->middle && node->type == NodeType::BINARY && node->_string == ":" && node->left->type == NodeType::BINARY && node->left->_string == ":")
                    {
                        Node left = node->left->left;
                        Node middle = node->left->right;
                        Node right = node->right;
                        node->left = left;
                        node->middle = middle;
                        node->right = right;
                    }
                }
                else
                    node->right = Ignore();
                return MaybeBinary(node, prec);
            }
        }
    }
    return left;
}

bool Parser::DelimitedNodes(std::vector<Node> &nodes, const std::string &start, const std::string &stop, const std::string &separator, bool op)
{
    bool first = true;
    if(!SkipSymbol(start) && (op && !SkipOperator(start)))
    {
        return false;
    }
    while(!tokens.Eof())
    {
        if(IsSymbol(stop) || (op && IsOperator(stop)))
            break;
        if(first)
            first = false;
        else
            SkipSymbol(separator);
        if(IsSymbol(stop) || (op && IsOperator(stop)))
            break;
        Node node = ParseExpression();
        if(node->type == NodeType::ERROR)
            return false;
        nodes.push_back(node);
    }
    if(!SkipSymbol(stop) && (op && !SkipOperator(stop)))
        return false;
    return true;
}

bool Parser::DelimitedNames(std::vector<string> &names, const std::string &start, const std::string &stop, const std::string &separator, bool op)
{
    bool first = true;
    if(!SkipSymbol(start) && (op && !SkipOperator(start)))
        return false;
    while(!tokens.Eof())
    {
        if(IsSymbol(stop) || (op && IsOperator(stop)))
            break;
        if(first)
            first = false;
        else
            SkipSymbol(separator);
        if(IsSymbol(stop) || (op && IsOperator(stop)))
            break;
        string name = ParseVarName();
        if(name.length() == 0)
            return false;
        names.push_back(name);
    }
    if(!SkipSymbol(stop) && (op && !SkipOperator(stop)))
        return false;
    return true;
}

Node Parser::ParseImport()
{
    Node node = MKNODE();
    node->type = NodeType::IMPORT;
    bool first = true;
    while(!tokens.Eof())
    {
        if(first)
            first = false;
        else
        {
            if(!SkipSymbol(","))
                break;
        }
        string name = ParseVarName();
        if(name.length() == 0)
        {
            MakeError(node, "Invalid import name", tokens.Peek());
            break;
        }
        Node var = MKNODE();
        var->type = NodeType::VARIABLE;
        var->_string = name;
        Token as = tokens.Peek();
        if(as.type == TokenType::KEYWORD && as._string == "as")
        {
            tokens.Next();
            string nick = ParseVarName();
            if(nick.length() == 0)
            {
                MakeError(node, "Invalid import nickname", tokens.Peek());
                break;
            }
            var->_nick = nick;
        }
        else if(as.type == TokenType::KEYWORD && as._string == "global")
        {
            tokens.Next();
            var->_nick = "__global__";
        }

        node->nodes.push_back(var);
    }
    return node;
}

Node Parser::ParseCall(Node func)
{
    Node node = MKNODE();
    node->type = NodeType::CALL;
    node->func = func;
    if(!DelimitedNodes(node->nodes, "(", ")", ","))
        MakeError(node, "Invalid function call", tokens.Peek());
    return node;
}

string Parser::ParseVarName()
{
    Token name = tokens.Peek();
    tokens.Next();
    if(name.type != TokenType::VARIABLE)
        return "";
    return name._string;
}

Node Parser::ParseIf()
{
    Node node = MKNODE();
    node->type = NodeType::IF;
    if(!SkipKeyword("if"))
    {
        MakeError(node, "Expected keyword 'if'", tokens.Peek());
        return node;
    }
    Node cond = ParseExpression();
    if(cond->type == NodeType::ERROR)
        return cond;
    Node then = ParseExpression();
    if(then->type == NodeType::ERROR)
        return then;
    node->cond = cond;
    node->then = then;
    if(IsKeyword("else"))
    {
        tokens.Next();
        Node contr = ParseExpression();
        if(contr->type == NodeType::ERROR)
            return contr;
        node->contr = contr;
    }
    return node;
}

Node Parser::ParseLet()
{
    Node node = MKNODE();
    node->type = NodeType::LET;
    if(!DelimitedNodes(node->nodes, "(", ")", ","))
    {
        MakeError(node, "Invalid let argument", tokens.Peek());
        return node;
    }
    Node body = ParseExpression();
    if(body->type == NodeType::ERROR)
        return body;
    node->body = body;
    return node;
}


Node Parser::ParseLambda()
{
    Node node = MKNODE();
    node->type = NodeType::LAMBDA;
    if(!DelimitedNames(node->vars, "(", ")", ","))
    {
        MakeError(node, "Invalid lambda argument", tokens.Peek());
        return node;
    }
    Node body = ParseExpression();
    if(body->type == NodeType::ERROR)
        return body;
    node->body = body;
    return node;
}

Node Parser::ParseFunction()
{
    Node node = MKNODE();
    node->type = NodeType::FUNCTION;

    Token name = tokens.Peek();
    node->_string = ParseVarName();
    if(IsOperator("."))
    {
        if(node->_string.length() == 0)
        {
            MakeError(node, "Invalid extension type name", name);
            return node;
        }
        tokens.Next();
        name = tokens.Peek();
        string func = ParseVarName();
        if(func.length() == 0)
        {
            MakeError(node, "Invalid extension name", name);
            return node;
        }
        node->type = NodeType::EXTENSION;
        node->_nick = node->_string;
        node->_string = func;
    }
    else
    {
        if(node->_string.length() == 0)
        {
            MakeError(node, "Invalid function name", name);
            return node;
        }
    }
    if(!DelimitedNames(node->vars, "(", ")", ","))
    {
        MakeError(node, "Invalid lambda argument", tokens.Peek());
        return node;
    }
    Node body = ParseExpression();
    if(body->type == NodeType::ERROR)
        return body;
    node->body = body;
    return node;
}

Node Parser::ParseTry()
{
    Node node = MKNODE();
    node->type = NodeType::TRY;
    Node then = ParseExpression();
    if(then->type == NodeType::ERROR)
        return then;
    if(SkipKeyword("catch"))
    {
        if(IsSymbol("("))
        {
            if(!DelimitedNames(node->vars, "(", ")", ",") || node->vars.size() > 1)
            {
                MakeError(node, "Invalid number of arguments for try/catch (must be 1)", tokens.Peek());
                return node;
            }
            node->_string = node->vars[0];
        }
        Node contr = ParseExpression();
        if(contr->type == NodeType::ERROR)
            return contr;
        node->contr = contr;
    }
    node->body = then;
    return node;
}

Node Parser::ParseFor()
{
    Node node = MKNODE();
    node->type = NodeType::FOR;
    if(!DelimitedNodes(node->nodes, "(", ")", ";"))
    {
        MakeError(node, "Invalid for arguments", tokens.Peek());
        return node;
    }
    Node body = ParseExpression();
    if(body->type == NodeType::ERROR)
        return body;
    node->body = body;
    return node;
}

Node Parser::ParseWhile()
{
    Node node = MKNODE();
    node->type = NodeType::WHILE;

    Node cond = ParseExpression();
    if(cond->type == NodeType::ERROR)
        return cond;
    Node body = ParseExpression();
    if(body->type == NodeType::ERROR)
        return body;
    node->cond = cond;
    node->body = body;

    return node;
}

Node Parser::ParseDoWhile()
{
    Node node = MKNODE();
    node->type = NodeType::DOWHILE;

    Node body = ParseExpression();
    if(body->type == NodeType::ERROR)
        return body;

    if(!SkipKeyword("while"))
    {
        MakeError(node, "Expected keyword 'while'", tokens.Peek());
        return node;
    }

    Node cond = ParseExpression();
    if(cond->type == NodeType::ERROR)
        return cond;
    node->cond = cond;
    node->body = body;

    return node;
}

Node Parser::ParseArray()
{
    Node node = MKNODE();
    if(!DelimitedNodes(node->nodes, "[", "]", ",", true))
        MakeError(node, "Invalid array definition", tokens.Peek());
    else
    {
        node->type = NodeType::DICT;
        for(int i = 0; i < node->nodes.size(); i++)
        {
            if(node->nodes[i]->type != NodeType::ASSIGN)
            {
                node->type = NodeType::ARRAY;
                break;
            }
            else if(node->nodes[i]->left->type != NodeType::VARIABLE)
            {
                MakeError(node, "Invalid dict key", tokens.Peek());
                return node;
            }
        }
    }
    return node;
}

Node Parser::ParseBool()
{
    Node node = MKNODE();
    node->type = NodeType::BOOL;
    node->_bool = tokens.Peek()._string == "true";
    tokens.Next();
    return node;
}

Node Parser::MaybeCall(Node expr)
{
    if(IsSymbol("("))
        return ParseCall(expr);
    return expr;
}

Node Parser::ParseAtom()
{
    Node node = MKNODE();
    if(IsSymbol("("))
    {
        tokens.Next();
        node = ParseExpression();
        if(node->type == NodeType::ERROR)
            return node;
        if(!SkipSymbol(")"))
        {
            MakeError(node, "Expected symbol ')'", tokens.Peek());
            return node;
        }
    }
    else if(IsSymbol("{"))
    {
        node = ParseContext();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("if"))
    {
        node = ParseIf();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("true") || IsKeyword("false"))
    {
        node = ParseBool();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("let"))
    {
        tokens.Next();
        node = ParseLet();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("@"))
    {
        tokens.Next();
        node = ParseLambda();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("func"))
    {
        tokens.Next();
        node = ParseFunction();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("for"))
    {
        tokens.Next();
        node = ParseFor();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("while"))
    {
        tokens.Next();
        node = ParseWhile();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("do"))
    {
        tokens.Next();
        node = ParseDoWhile();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("return"))
    {
        tokens.Next();
        node->type = NodeType::RETURN;
        node->body = ParseExpression();
        if(node->body->type == NodeType::ERROR)
            return node->body;
    }
    else if(IsKeyword("import"))
    {
        tokens.Next();
        node = ParseImport();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("try"))
    {
        tokens.Next();
        node = ParseTry();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsKeyword("none"))
    {
        tokens.Next();
        node->type = NodeType::NONE;
    }
    else if(IsOperator() && noleft.count(tokens.Peek()._string) > 0)
    {
        node = MaybeBinary(Ignore(), 0);
        if(node->type == NodeType::ERROR)
            return node;
    }
    else if(IsOperator("["))
    {
        node = ParseArray();
        if(node->type == NodeType::ERROR)
            return node;
    }
    else
    {
        Token tok = tokens.Peek();
        tokens.Next();
        if(tok.type == TokenType::VARIABLE)
        {
            node->type = NodeType::VARIABLE;
            node->_string = tok._string;
        }
        else if(tok.type == TokenType::NUMBER)
        {
            node->type = NodeType::NUMBER;
            node->_number = tok._number;
        }
        else if(tok.type == TokenType::STRING)
        {
            node->type = NodeType::STRING;
            node->_string = tok._string;
        }
        else
        {
            MakeError(node, "Unexpected", tok);
            return node;
        }
    }

    return MaybeCall(node);
}

Node Parser::ParseContext()
{
    Node node = MKNODE();
    node->type = NodeType::CONTEXT;
    if(!DelimitedNodes(node->nodes, "{", "}", ";"))
    {
        MakeError(node, "Unexpected symbol in context", tokens.Peek());
        return node;
    }
    if(node->nodes.size() == 0)
    {
        return False();
    }
    else if(node->nodes.size() == 1)
    {
        return node->nodes[0];
    }
    return node;
}

Node Parser::False()
{
    Node node = MKNODE();
    node->type = NodeType::BOOL;
    node->_bool = false;
    return node;
}

Node Parser::Ignore()
{
    Node node = MKNODE();
    node->type = NodeType::IGNORE;
    return node;
}

void Parser::MakeError(Node &node, const string& msg, Token &token)
{
    stringstream ss;
    ss << msg;
    ss << " { " << token.ToString() << " } ";
    ss << " [Line " << token.row << ", Column: " << token.col << "]";
    node->_string = ss.str();
    node->type = NodeType::ERROR;
}

bool Parser::HasOperator(const std::string& op)
{
    map<string, int>::iterator it = precedence.find(op);
    return it != precedence.end();
}

bool Parser::IsSymbol(const string& symbol)
{
    Token token = tokens.Peek();
    bool ret = token.type == TokenType::SYMBOL;
    if(symbol.size() > 0)
        ret &= token._string == symbol;
    return  ret;
}

bool Parser::IsKeyword(const string& keyword)
{
    Token token = tokens.Peek();
    bool ret = token.type == TokenType::KEYWORD;
    if(keyword.size() > 0)
        ret &= token._string == keyword;
    return  ret;
}

bool Parser::IsOperator(const string& op)
{
    Token token = tokens.Peek();
    bool ret = token.type == TokenType::OPERATOR;
    if(op.size() > 0)
        ret &= token._string == op;
    return  ret;
}

bool Parser::SkipSymbol(const string& symbol)
{
    if(IsSymbol(symbol))
    {
        tokens.Next();
        return true;
    }
    return false;
}

bool Parser::SkipKeyword(const string& keyword)
{
    if(IsKeyword(keyword))
    {
        tokens.Next();
        return true;
    }
    return false;
}

bool Parser::SkipOperator(const string& op)
{
    if(IsOperator(op))
    {
        tokens.Next();
        return true;
    }
    return false;
}