#include "token/token.hpp"
#include "ast.hpp"
#include <string>
#include <vector>
#include <map>
using namespace std;

class Parser
{
    vector<Token> tokenInput;
    int currentPos;
    int nextPos;

    // Precedence and token type map
    map<TokenType, Precedence> precedence{
        {TokenType::ASSIGN, Precedence::PREC_ASSIGNMENT},
        {TokenType::OR, Precedence::PREC_OR},
        {TokenType::AND, Precedence::PREC_AND},
        {TokenType::EQUALS, Precedence::PREC_EQUALITY},
        {TokenType::NOT_EQUALS, Precedence::PREC_EQUALITY},
        {TokenType::GREATER_THAN, Precedence::PREC_COMPARISON},
        {TokenType::LESS_THAN, Precedence::PREC_COMPARISON},
        {TokenType::GT_OR_EQ, Precedence::PREC_COMPARISON},
        {TokenType::LT_OR_EQ, Precedence::PREC_COMPARISON},
        {TokenType::PLUS, Precedence::PREC_TERM},
        {TokenType::MINUS, Precedence::PREC_TERM},
        {TokenType::ASTERISK, Precedence::PREC_FACTOR},
        {TokenType::DIVIDE, Precedence::PREC_FACTOR},
        {TokenType::BANG, Precedence::PREC_UNARY},
        {TokenType::FULLSTOP, Precedence::PREC_CALL},
        {TokenType::IDENTIFIER, Precedence::PREC_PRIMARY},
    };

public:
    // Parser class declaration
    Parser(vector<Token> &tokenInput);
    // Main parser program
    vector<unique_ptr<Node>> parseProgram();

    // Sliding across the token input from the lexer;
    void advance();

    // Functions for registering the functions neccesary for parsing the different tokens
    void registerPrefixFns();
    void registerInfixFns();

    // Function to register keyword parsing functions
    void registerKeywordParseFns();

    // Function to register statement parsing functions
    void registerStatementParseFns();

    // Function to get the precedence depending on the token type
    Precedence get_precedence(TokenType type);

    using prefixParseFns = unique_ptr<Expression> (Parser::*)();
    using infixParseFns = unique_ptr<Expression> (Parser::*)(unique_ptr<Expression>);

    using keywordParseFns = unique_ptr<Statement> (Parser::*)();
    using stmtParseFns = unique_ptr<Statement> (Parser::*)();

    map<TokenType, prefixParseFns> PrefixParseFunctionsMap;
    map<TokenType, infixParseFns> InfixParseFunctionsMap;

    map<TokenType, keywordParseFns> KeywordParseFunctionsMap;
    map<TokenType, stmtParseFns> StatementParseFunctionsMap;

private:
    //---------------PARSING STATEMENTS--------------------
    // General statement parsing function
    unique_ptr<Statement> parseStatement();

    // Parsing let statements with type
    unique_ptr<Statement> parseLetStatementWithType();

    // Parsing let statements without type
    unique_ptr<Statement> parseLetStatementWithoutType();

    // Parsing if statement
    unique_ptr<Statement> parseIfStatement();

    // Parsing return statements
    unique_ptr<Statement> parseReturnStatement();

    //Parsing for statement
    unique_ptr<Statement> parseForStatement();

    //Parsing while loops
    unique_ptr<Statement> parseWhileStatement();

    // Parsing block statements
    unique_ptr<Statement> parseBlockStatement();

    // Parsing grouped expressions
    unique_ptr<Expression> parseGroupedExpression();

    //--------------PARSING EXPRESSIONS--------------------
    // Main expression parsing function
    unique_ptr<Expression> parseExpression(Precedence precedence);

    // Infix expression parsing function
    unique_ptr<Expression> parseInfixExpression(unique_ptr<Expression> left);

    // Prefix expression parsing function
    unique_ptr<Expression> parsePrefixExpression();

    // Parsing identifiers
    unique_ptr<Expression> parseIdentifier();

    // Parsing block expressions
    unique_ptr<Expression> parseBlockExpression();

    // Parsing data type literals
    // Integer
    unique_ptr<Expression> parseIntegerLiteral();

    // Boolean
    unique_ptr<Expression> parseBooleanLiteral();

    // Float
    unique_ptr<Expression> parseFloatLiteral();

    // Char
    unique_ptr<Expression> parseCharLiteral();

    // String
    unique_ptr<Expression> parseStringLiteral();

    // Peeking functions
    Token currentToken();
    Token nextToken();

    // Checking for the statement start
    bool isStatementStart(const Token &token);
};