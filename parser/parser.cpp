#include "parser.hpp"
#include "ast.hpp"
#include "token/token.hpp"
#include <iostream>
#include <memory>
#include <vector>
using namespace std;

//--------------PARSER CLASS CONSTRUCTOR-------------
Parser::Parser(vector<Token> &tokenInput) : tokenInput(tokenInput), currentPos(0), nextPos(1)
{
    lastToken = tokenInput.empty() ? Token{"", TokenType::ILLEGAL, 999, 999} : tokenInput[0];
    registerInfixFns();
    registerPrefixFns();
    registerStatementParseFns();
}

// MAIN PARSER FUNCTION
vector<unique_ptr<Node>> Parser::parseProgram()
{
    vector<unique_ptr<Node>> program;

    while (currentPos < tokenInput.size())
    {
        cout << "Parsing token: " << currentToken().TokenLiteral << endl;
        Token current = currentToken();

        if (current.type == TokenType::END)
        {
            break;
        }

        unique_ptr<Node> node = parseStatement();

        if (node)
        {
            program.push_back(std::move(node));
        }
        else
        {
            advance();
        }
    }

    cout << "Parser finished" << endl;
    return program;
}

//------------PARSING FUNCTIONS SECTION----------
//-----------PARSING STATEMENTS----------
// General statement parser function
unique_ptr<Statement> Parser::parseStatement()
{
    Token current = currentToken();
    std::cout << "[DEBUG] parseStatement starting with token: " << current.TokenLiteral << std::endl;
    if (current.type == TokenType::SEMICOLON)
    {
        advance();
        return nullptr;
    }

    if (current.type == TokenType::IDENTIFIER)
    {
        Token peek_token = nextToken();
        if (peek_token.type == TokenType::ASSIGN)
        {
            return parseAssignmentStatement();
        }
    }

    auto stmtFnIt = StatementParseFunctionsMap.find(current.type);

    if (stmtFnIt != StatementParseFunctionsMap.end())
    {
        auto stmt = (this->*stmtFnIt->second)();
        return stmt;
    }

    auto expr = parseExpression(Precedence::PREC_NONE);
    if (expr)
    {
        if (currentToken().type == TokenType::SEMICOLON)
        {
            advance();
        }
        else
        {
            logError("Expected ';' after expression statement");
            return nullptr;
        }

        std::cout << "[DEBUG] Parsed generic expression statement.\n";
        return make_unique<ExpressionStatement>(current, move(expr));
    }

    logError("Unexpected token at start of statement:  ");
    advance();
    return nullptr;
}

// Parsing let statements with types
unique_ptr<Statement> Parser::parseAssignmentStatement(bool isParam)
{
    Token ident_token = currentToken();
    cout << "[DEBUG] Identifier token: " + ident_token.TokenLiteral << endl;
    advance();

    if (currentToken().type != TokenType::ASSIGN)
    {
        logError("Expected = after identifier");
        return nullptr;
    }
    advance();

    unique_ptr<Expression> value = parseExpression(Precedence::PREC_NONE);

    if (!isParam && currentToken().type == TokenType::SEMICOLON)
    {
        advance();
    }
    else
    {
        logError("Expected a semi colon ");
    }

    return make_unique<AssignmentStatement>(ident_token, move(value));
}

// Parsing let statements that have data types
unique_ptr<Statement> Parser::parseLetStatementWithType(bool isParam)
{
    Token dataType_token = currentToken();
    cout << "[DEBUG] Data type token: " + dataType_token.TokenLiteral << endl;
    advance();

    if (currentToken().type != TokenType::IDENTIFIER)
    {
        logError("Expected variable name after data type ");
        return nullptr;
    }

    Token ident_token = currentToken();
    advance();

    optional<Token> assign_token;
    unique_ptr<Expression> value = nullptr;

    if (currentToken().type == TokenType::ASSIGN)
    {
        assign_token = currentToken();
        cout << "[DEBUG] Encountered assignment token" << endl;
        advance();
        value = parseExpression(Precedence::PREC_NONE);
    }
    else if (currentToken().type == TokenType::SEMICOLON)
    {
        cout << "[DEBUG] Encountered semicolon token" << endl;
    }

    if (!isParam && currentToken().type == TokenType::SEMICOLON)
    {
        advance();
    }
    else
    {
        logError("Expected a semi colon");
    }

    return make_unique<LetStatement>(dataType_token, ident_token, assign_token, move(value));
}

/*Decider on type of let statement: Now the name of this function is confusing initially I wanted it to be the function that decides how to parse let statements.
But I have no choice  but to make it also decide how to parse a function if it was a parameter now I will fix the name in the future but for now let me just continue
*/
std::unique_ptr<Statement> Parser::parseLetStatementDecider()
{
    Token current = currentToken();

    if (current.type == TokenType::INT ||
        current.type == TokenType::FLOAT_KEYWORD ||
        current.type == TokenType::STRING_KEYWORD ||
        current.type == TokenType::CHAR_KEYWORD ||
        current.type == TokenType::BOOL_KEYWORD ||
        current.type == TokenType::AUTO)
    {
        // TODO: Will add support for functions as parameters later for now will only supoort simple parameters
        return parseLetStatementWithType(true);
    }
    else if (
        current.type == TokenType::IDENTIFIER)
    {
        if (nextToken().type == TokenType::ASSIGN)
        {
            return parseAssignmentStatement(true);
        }
    }
    std::cerr << "[ERROR]: Failed to decide how to parse parameter variable. Token: " << current.TokenLiteral << "\n";
    return nullptr;
}

// Parsing signal statement
std::unique_ptr<Statement> Parser::parseSignalStatement()
{
    Token signal_token = currentToken();
    advance();
    auto ident = parseIdentifier();
    if (currentToken().type != TokenType::ASSIGN)
    {
        logError("Expected an =");
        return nullptr;
    }
    advance(); // Move past the = sign
    if (currentToken().type != TokenType::START)
    {
        logError("Expected a thread starter");
        return nullptr;
    }
    auto start = parseStartStatement();
    if (currentToken().type != TokenType::LPAREN)
    {
        logError("Expected ( after start keyword");
        return nullptr;
    }
    advance();
    auto func_name = parseIdentifier();
    auto func_arg = parseCallExpression(std::move(func_name));
    advance();
    if (currentToken().type != TokenType::SEMICOLON)
    {
        cout << "[ERROR]: Expected ; after ) but got->" << currentToken().TokenLiteral << "\n";
        logError("Expected ; after )");
        return nullptr;
    }

    return make_unique<SignalStatement>(signal_token, std::move(ident), std::move(start), std::move(func_arg));
}

// Parsing start statement
std::unique_ptr<Statement> Parser::parseStartStatement()
{
    Token start = currentToken();
    advance();
    return make_unique<StartStatement>(start);
}

// Parsing wait statement
std::unique_ptr<Statement> Parser::parseWaitStatement()
{
    Token wait = currentToken();
    advance();
    if (currentToken().type != TokenType::LPAREN)
    {
        logError("Expected ( after wait keyword");
        return nullptr;
    }
    advance();
    auto call = parseIdentifier();
    if (currentToken().type != TokenType::RPAREN)
    {
        logError("Expected ) after argument");
        return nullptr;
    }
    advance();
    if (currentToken().type != TokenType::SEMICOLON)
    {
        logError("Expected ; after )");
        return nullptr;
    }
    return make_unique<WaitStatement>(wait, std::move(call));
}

// Parsing function statement
std::unique_ptr<Statement> Parser::parseFunctionStatement()
{
    Token funcToken=currentToken();
  
    std::unique_ptr<Expression>funcExpr=parseFunctionExpression();
    if(!funcExpr){
        return nullptr;
    }
    
    return make_unique<FunctionStatement>(funcToken,std::move(funcExpr));
}

// Parsing return statements
unique_ptr<Statement> Parser::parseReturnStatement()
{
    Token return_stmt = currentToken();
    advance();

    if (currentToken().type == TokenType::SEMICOLON || currentToken().type == TokenType::END)
    {
        logError("Return is void");
        return make_unique<ReturnStatement>(return_stmt, nullptr);
    }

    cout << "[DEBUG] Parsing return expression token: " << currentToken().TokenLiteral << endl;
    auto return_value = parseExpression(Precedence::PREC_NONE);

    if (!return_value)
    {
        logError("Return Value is NULL");
    }

    if (currentToken().type == TokenType::SEMICOLON)
    {
        advance();
    }

    return make_unique<ReturnStatement>(return_stmt, move(return_value));
}

// Parse for loops
unique_ptr<Statement> Parser::parseForStatement()
{
    Token for_k = currentToken();
    advance();
    if (currentToken().type != TokenType::LPAREN)
    {
        logError("Expected a ( ");
        return nullptr;
    }
    advance();
    auto initializer = parseLetStatementWithType(); // like `int i;`

    auto condition = parseExpression(Precedence::PREC_NONE);

    if (currentToken().type != TokenType::SEMICOLON)
    {
        logError("Expected ';' after condition");
        return nullptr;
    }
    advance(); // skip ';'

    auto step = parseExpression(Precedence::PREC_NONE);
    if (currentToken().type != TokenType::RPAREN)
    {
        logError("Expected ')' after step");
        return nullptr;
    }
    advance(); // skip ')'

    if (currentToken().type != TokenType::LBRACE)
    {
        logError("Expected { ");
        return nullptr;
    }
    auto block = parseBlockStatement(); // Parsing the block

    return std::make_unique<ForStatement>(
        for_k,
        std::move(initializer),
        std::move(condition),
        std::move(step),
        std::move(block));
}

// Parsing while statements
unique_ptr<Statement> Parser::parseWhileStatement()
{
    Token while_key = currentToken();
    advance();
    if (currentToken().type != TokenType::LPAREN)
    {
        logError(" Expected ')' after keyword while ");
        return nullptr;
    }
    advance();
    auto condition = parseExpression(Precedence::PREC_NONE);
    if (currentToken().type != TokenType::RPAREN)
    {
        logError(" Expected ')' after while condition");
        return nullptr;
    }
    advance();
    auto result = parseBlockStatement();
    return make_unique<WhileStatement>(while_key, move(condition), move(result));
}

// Parse break statement
std::unique_ptr<Statement> Parser::parseBreakStatement()
{
    Token break_tok = currentToken();
    advance();
    advance();
    return make_unique<BreakStatement>(break_tok);
}

// Parse continue statement
std::unique_ptr<Statement> Parser::parseContinueStatement()
{
    Token cont_tok = currentToken();
    advance();
    advance();
    return make_unique<ContinueStatement>(cont_tok);
}

// Parsing if statements`
unique_ptr<Statement> Parser::parseIfStatement()
{
    Token if_stmt = currentToken();
    advance();

    if (currentToken().type != TokenType::LPAREN)
    {
        logError("Expected '(' after 'if' ");
        return nullptr;
    }
    advance();

    auto condition = parseExpression(Precedence::PREC_NONE);
    if (currentToken().type != TokenType::RPAREN)
    {
        cout << "[DEBUG] Expected ')' got: " << currentToken().TokenLiteral << endl;
        logError("Expected ')' got: ");
        return nullptr;
    }
    advance();
    auto if_result = parseBlockStatement();

    optional<Token> elseif_stmt;
    optional<unique_ptr<Expression>> elseif_condition;
    optional<unique_ptr<Statement>> elseif_result;

    if (currentToken().type == TokenType::ELSE_IF)
    {
        elseif_stmt = currentToken();
        advance();

        if (currentToken().type != TokenType::LPAREN)
        {
            cout << "[DEBUG] Expected '(' after 'elseif', got: " << currentToken().TokenLiteral << endl;
            logError("Expected '(' after 'elseif'");
            return nullptr;
        }

        elseif_condition = parseGroupedExpression();
        elseif_result = parseBlockStatement();
    }

    optional<Token> else_stmt;
    optional<unique_ptr<Statement>> else_result;

    if (currentToken().type == TokenType::ELSE)
    {
        else_stmt = currentToken();
        advance();

        else_result = parseBlockStatement();
    }

    return make_unique<ifStatement>(
        if_stmt,
        std::move(condition),
        std::move(if_result),
        std::move(elseif_stmt),
        std::move(elseif_condition),
        std::move(elseif_result),
        std::move(else_stmt),
        std::move(else_result));
}

// Parsing identifiers
unique_ptr<Expression> Parser::parseIdentifier()
{
    auto ident = make_unique<Identifier>(currentToken());
    if (!ident)
    {
        logError("Failed to parse identifier: ");
    }
    advance();
    return ident;
}

//-----------PARSING EXPRESSIONS----------
// Expression parsing section
// Main Expression parsing function
unique_ptr<Expression> Parser::parseExpression(Precedence precedence)
{
    auto PrefixParseFnIt = PrefixParseFunctionsMap.find(currentToken().type); // Creating an iterator pointer to loop through the map

    if (PrefixParseFnIt == PrefixParseFunctionsMap.end()) // Checking if the iterator has reached the end of the map
    {
        cerr << "[ERROR] No prefix parse function for token: " << currentToken().TokenLiteral << endl;
        return nullptr;
    }

    auto left_expression = (this->*PrefixParseFnIt->second)(); // Calling the neccesary prefix function after encountering that particular token
    cout << "[DEBUG] Initial left expression: " << left_expression->toString() << endl;

    while (precedence < get_precedence(currentToken().type)) // Looping as long as the precedence is lower than the precedence of the current token
    {
        cout << "[DEBUG] Looping for token: " << currentToken().TokenLiteral << endl;
        auto InfixParseFnIt = InfixParseFunctionsMap.find(currentToken().type); // Creating the iterator pointer for the infix map

        if (InfixParseFnIt == InfixParseFunctionsMap.end()) // Checking if the iterator has reached the end of the map
        {
            cout << "[DEBUG] No infix parser found for: " << currentToken().TokenLiteral << endl;
            break;
        }

        left_expression = (this->*InfixParseFnIt->second)(std::move(left_expression)); // If we find the infix parse function for that token we call the function
        cout << "[DEBUG] Updated left expression: " << left_expression->toString() << endl;
    }

    return left_expression; // Returning the expression that was parsed it can be either prefix or infix
}

// Inifix parse function definition
unique_ptr<Expression> Parser::parseInfixExpression(unique_ptr<Expression> left)
{
    Token operat = currentToken();
    cout << "[DEBUG] parsing infix with operator: " << operat.TokenLiteral << endl;
    Precedence prec = get_precedence(operat.type);
    advance();
    auto right = parseExpression(prec);
    return make_unique<InfixExpression>(move(left), operat, move(right));
}

// Prefix parse function definition
unique_ptr<Expression> Parser::parsePrefixExpression()
{
    Token operat = currentToken();
    Precedence operatorPrecedence = get_precedence(operat.type);
    advance();
    auto operand = parseExpression(operatorPrecedence);
    return make_unique<PrefixExpression>(operat, move(operand));
}

// Integer literal parse function
unique_ptr<Expression> Parser::parseIntegerLiteral()
{
    auto ident = make_unique<IntegerLiteral>(currentToken());
    advance();
    return ident;
}

// Boolean literal parse function
unique_ptr<Expression> Parser::parseBooleanLiteral()
{
    Token bool_tok = currentToken();
    advance();
    return make_unique<BooleanLiteral>(bool_tok);
}

// Float literal parse function
unique_ptr<Expression> Parser::parseFloatLiteral()
{
    Token float_tok = currentToken();
    advance();
    return make_unique<FloatLiteral>(float_tok);
}

// Char literal parse function
unique_ptr<Expression> Parser::parseCharLiteral()
{
    Token char_tok = currentToken();
    advance();
    return make_unique<CharLiteral>(char_tok);
}

// String literal parse function
unique_ptr<Expression> Parser::parseStringLiteral()
{
    Token string_tok = currentToken();
    advance();
    return make_unique<StringLiteral>(string_tok);
}

// Grouped expression parse function
unique_ptr<Expression> Parser::parseGroupedExpression()
{
    Token lparen = currentToken();
    advance();

    if (currentToken().type == TokenType::RPAREN)
    {
        std::cerr << "[ERROR] Empty grouped expression after '('\n";
        logError("Empty grouped expression after '('");
        return nullptr;
    }

    auto expr = parseExpression(Precedence::PREC_NONE);
    if (!expr)
    {
        std::cerr << "[ERROR] Failed to parse expression inside grouped expr.\n";
        logError("Empty grouped expression after '('");
        return nullptr;
    }

    if (currentToken().type != TokenType::RPAREN)
    {
        std::cerr << "[ERROR] Expected ')' to close grouped expression, got: " << currentToken().TokenLiteral << endl;
        logError("Expected ')' to close grouped expression ");
        return nullptr;
    }

    advance();
    return expr;
}

unique_ptr<Expression> Parser::parseCallExpression(unique_ptr<Expression> left)
{
    cout << "[DEBUG] Entered parseCallExpression for: " << left->toString() << "\n";
    Token call_token = currentToken(); // We expect a left parenthesis here

    if (call_token.type != TokenType::LPAREN)
    { // Checking if we encounter the left parenthesis after the function name has been declared
        logError("Expected ( after function name");
        return nullptr;
    }

    advance(); // Advancing the pointer to look at what is inside the brackets

    auto args = parseCallArguments(); // Calling the parse call arguments inorder to parse the arguments

    return make_unique<CallExpression>(call_token, move(left), move(args));
}

// Parsing function call arguments

vector<unique_ptr<Expression>> Parser::parseCallArguments()
{
    vector<unique_ptr<Expression>> args;
    if (currentToken().type == TokenType::RPAREN)
    {
        advance();
        return args;
    }

    auto firstArg = parseExpression(Precedence::PREC_NONE);
    if (!firstArg)
    {
        cerr << "Failed to parse first function argument.\n";
        return args;
    }
    args.push_back(std::move(firstArg));

    while (currentToken().type == TokenType::COMMA)
    {
        advance();
        auto arg = parseExpression(Precedence::PREC_NONE);
        if (!arg)
        {
            cerr << "Failed to parse function argument after comma.\n";
            return args;
        }
        args.push_back(std::move(arg));
    }

    if (currentToken().type == TokenType::RPAREN)
    {
        advance(); // consume ')'
    }
    else
    {
        logError("Expected ')' after function arguments");
    }

    return args;
}

// Parsing function expression
unique_ptr<Expression> Parser::parseFunctionExpression()
{
    cout << "[TEST]Function parser is working\n";
    //--------Dealing with work keyword---------------
    Token func_tok = currentToken(); // The token represting the keyword for functions (work)
    advance();

    //----------Dealing with function name------------
    if (currentToken().type != TokenType::IDENTIFIER)
    {
        logError("Expected function name after keyword work");
        return nullptr;
    }
    auto func_name = parseIdentifier();

    //---Dealing with the call itself
    auto call = parseFunctionParameters(); // We might get some arguments or not so we call the parse call expression

    unique_ptr<Expression> return_type = nullptr;
    //--Checking for colons
    if (currentToken().type == TokenType::COLON)
    {
        advance(); // Move past the colon signs
        switch (currentToken().type)
        {
        case TokenType::INT:
        case TokenType::FLOAT_KEYWORD:
        case TokenType::STRING_KEYWORD:
        case TokenType::BOOL_KEYWORD:
        case TokenType::AUTO:
        case TokenType::VOID:
            return_type = make_unique<ReturnTypeExpression>(currentToken());
            advance();
            break;
        default:
            logError("Unexpected return type: ");
            return nullptr;
        }
    }

    std::cout << "[DEBUG]: Encountered the " << currentToken().TokenLiteral << "\n";
    
    auto block = parseBlockExpression(); // Parsing the blocks
    if (!block)
    {
        cerr << "[ERROR] Failed to parse function body.\n";
        return nullptr;
    }

    return make_unique<FunctionExpression>(func_tok, move(call), move(return_type), move(block));
}

// Parsing function patamemters
vector<unique_ptr<Statement>> Parser::parseFunctionParameters()
{
    std::cout << "PARSING FUNCTION PARAMETERS\n";
    std::vector<std::unique_ptr<Statement>> args; // Decldaring the empty vector

    // Checking if the current token is the lparen
    if (currentToken().type != TokenType::LPAREN)
    {
        logError("Expected a ( to start a function parameter ");
        return args;
    }

    // Advancing from the lparen
    advance();

    if (currentToken().type == TokenType::RPAREN)
    {
        advance();
        if (currentToken().type != TokenType::COLON)
        {
            logError("Expected ':' after empty parameter list ");
        }

        return args; // Return an empty vector of arguments
    }

    auto firstParam = parseLetStatementDecider(); // Parsing the fisrt parameter i.e 1

    if (!firstParam)
    { // Checking if it failed to parse the 1st parameter
        cerr << "Failed to parse first parameter.\n";
        return args;
    }
    args.push_back(move(firstParam)); // If its parsed we add it to the vector

    while (currentToken().type == TokenType::COMMA)
    {                                          // If we still have commas
        advance();                             // Advance from the comma to the second parameter
        auto arg = parseLetStatementDecider(); // Parse the second parameter
        if (!arg)
        { // It it fails to parse the second parameter
            cerr << "Failed to parse parameter after comma\n";
            return args;
        }
        args.push_back(move(arg));
    }

    if (currentToken().type != TokenType::RPAREN)
    {
        logError("Expected ')' after function parameters ");
        return args;
    }
    advance();

    //--THIS IS WHERE THE FUNCTION HAS TO DEFER FROM PARSE CALL EXPRESSION
    if (currentToken().type != TokenType::COLON)
    {
        logError("Expected : after function closing ");
    }

    return args;
}

// Parsing block expressions
unique_ptr<Expression> Parser::parseBlockExpression()
{
    Token lbrace = currentToken();
    if (lbrace.type != TokenType::LBRACE)
    {
        logError("Expexted { after data type");
        return nullptr;
    }
    advance();
    auto block = make_unique<BlockExpression>(lbrace);
    while (currentToken().type != TokenType::RBRACE)
    {
        if (currentToken().type == TokenType::END)
        {
            logError("Unterminated block experession");
            return nullptr;
        }

        auto stmt = parseStatement();
        if (stmt)
        {
            block->statements.push_back(move(stmt));
        }
        else
        {
            auto expr = parseExpression(Precedence::PREC_NONE);
            if (expr)
            {
                block->finalexpr = move(expr);
            }
            break;
        }
    }

    if (currentToken().type != TokenType::RBRACE)
    {
        logError("Expected } ");
        return nullptr;
    }

    advance();
    return block;
}

// Parsing block statements
unique_ptr<Statement> Parser::parseBlockStatement()
{
    Token lbrace = currentToken();
    if (lbrace.type != TokenType::LBRACE)
    {
        logError("[ERROR] Expected '{' to start block");
        return nullptr;
    }
    advance();
    vector<unique_ptr<Statement>> statements;

    while (currentToken().type != TokenType::RBRACE && currentToken().type != TokenType::END)
    {
        auto stmt = parseStatement();
        if (stmt != nullptr)
        {
            statements.push_back(std::move(stmt));
        }
        else
        {
            cerr << "[ERROR] Failed to parse statement within block. Skipping token: " << currentToken().TokenLiteral << endl;
            advance();
        }
    }

    if (currentToken().type != TokenType::RBRACE)
    {
        logError("Expected } to close block");
        return nullptr;
    }

    advance();

    return make_unique<BlockStatement>(lbrace, std::move(statements));
}

//----------HELPER FUNCTIONS---------------
// Slider function
void Parser::advance()
{
    if (nextPos < tokenInput.size())
    {
        lastToken = currentToken();
        currentPos = nextPos;
        nextPos++;
        std::cout << "[TRACE]: Advanced to token: " << currentToken().TokenLiteral << "\n";
    }
}

// Registration functions
// Registering infix functions for a particular token type
void Parser::registerInfixFns()
{
    InfixParseFunctionsMap[TokenType::PLUS] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::MINUS] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::DIVIDE] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::ASTERISK] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::MODULUS] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::GREATER_THAN] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::LESS_THAN] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::GT_OR_EQ] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::LT_OR_EQ] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::AND] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::OR] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::NOT_EQUALS] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::EQUALS] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::ASSIGN] = &Parser::parseInfixExpression;
    InfixParseFunctionsMap[TokenType::LPAREN] = &Parser::parseCallExpression;
}

// Registering prefix functions for a particular token type
void Parser::registerPrefixFns()
{
    PrefixParseFunctionsMap[TokenType::INTEGER] = &Parser::parseIntegerLiteral;
    PrefixParseFunctionsMap[TokenType::TRUE] = &Parser::parseBooleanLiteral;
    PrefixParseFunctionsMap[TokenType::FALSE] = &Parser::parseBooleanLiteral;
    PrefixParseFunctionsMap[TokenType::FLOAT] = &Parser::parseFloatLiteral;
    PrefixParseFunctionsMap[TokenType::CHAR] = &Parser::parseCharLiteral;
    PrefixParseFunctionsMap[TokenType::STRING] = &Parser::parseStringLiteral;
    PrefixParseFunctionsMap[TokenType::IDENTIFIER] = &Parser::parseIdentifier;
    PrefixParseFunctionsMap[TokenType::BANG] = &Parser::parsePrefixExpression;
    PrefixParseFunctionsMap[TokenType::MINUS] = &Parser::parsePrefixExpression;
    PrefixParseFunctionsMap[TokenType::LPAREN] = &Parser::parseGroupedExpression;
    PrefixParseFunctionsMap[TokenType::LBRACE] = &Parser::parseBlockExpression;
    PrefixParseFunctionsMap[TokenType::PLUS_PLUS] = &Parser::parsePrefixExpression;
    PrefixParseFunctionsMap[TokenType::MINUS_MINUS] = &Parser::parsePrefixExpression;
}

// Wrapper function for letstatement with type
std::unique_ptr<Statement> Parser::parseLetStatementWithTypeWrapper()
{
    return parseLetStatementWithType();
}

// Registering the statement parsing functions
void Parser::registerStatementParseFns()
{
    StatementParseFunctionsMap[TokenType::ASSIGN] = &Parser::parseLetStatementDecider;
    StatementParseFunctionsMap[TokenType::RETURN] = &Parser::parseReturnStatement;
    StatementParseFunctionsMap[TokenType::IF] = &Parser::parseIfStatement;
    StatementParseFunctionsMap[TokenType::WHILE] = &Parser::parseWhileStatement;
    StatementParseFunctionsMap[TokenType::FOR] = &Parser::parseForStatement;
    StatementParseFunctionsMap[TokenType::BREAK] = &Parser::parseBreakStatement;
    StatementParseFunctionsMap[TokenType::CONTINUE] = &Parser::parseContinueStatement;
    StatementParseFunctionsMap[TokenType::SIGNAL] = &Parser::parseSignalStatement;
    StatementParseFunctionsMap[TokenType::START] = &Parser::parseStartStatement;
    StatementParseFunctionsMap[TokenType::WAIT] = &Parser::parseWaitStatement;
    StatementParseFunctionsMap[TokenType::INT] = &Parser::parseLetStatementWithTypeWrapper;
    StatementParseFunctionsMap[TokenType::FLOAT_KEYWORD] = &Parser::parseLetStatementWithTypeWrapper;
    StatementParseFunctionsMap[TokenType::STRING_KEYWORD] = &Parser::parseLetStatementWithTypeWrapper;
    StatementParseFunctionsMap[TokenType::BOOL_KEYWORD] = &Parser::parseLetStatementWithTypeWrapper;
    StatementParseFunctionsMap[TokenType::CHAR_KEYWORD]=&Parser::parseLetStatementWithTypeWrapper;
    StatementParseFunctionsMap[TokenType::FUNCTION]=&Parser::parseFunctionStatement;
    StatementParseFunctionsMap[TokenType::AUTO] = &Parser::parseLetStatementWithTypeWrapper;
}

// Precedence getting function
Precedence Parser::get_precedence(TokenType type)
{
    Precedence prec = precedence.count(type) ? precedence[type] : Precedence::PREC_NONE;
    return prec;
}

// Current token peeking function
Token Parser::currentToken()
{
    Token current = tokenInput[currentPos];
    return current;
}

// Next token peeking function
Token Parser::nextToken()
{
    Token next = tokenInput[nextPos];
    return next;
}

// Error logging
void Parser::logError(const std::string &message)
{
    Token token = getErrorToken();
    if (token.line == 0 && token.column == 0)
    {
        std::cerr << "[PANIC]: Logging an uninitialized token! Investigate token flow.\n";
    }
    std::cerr << "[PARSER ERROR]: " << message << " At line: " << token.line << " column: " << token.column << "\n";
    errors.push_back(
        ParseError{
            message,
            token.line,
            token.column});
}

// Getting the error token
Token Parser::getErrorToken()
{
    if (currentPos >= tokenInput.size())
    {
        if (lastToken.line == 0)
        {
            return Token{"", TokenType::ILLEGAL, 999, 999}; 
        }
        return lastToken;
    }
    return tokenInput[currentPos-1];
}