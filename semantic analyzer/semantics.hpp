#pragma once
#include <memory>
#include <map>
#include <string>
#include <typeindex>
#include "ast.hpp"

//Type system
enum class TypeSystem{
    INTEGER,
    FLOAT,
    BOOLEAN,
    STRING,
    CHAR,
    UNKNOWN,
};

std::string TypeSystemStr(TypeSystem type);

//Meta data struct that will be attached to each node after semantic analysis
struct SemanticInfo{
    TypeSystem nodeType = TypeSystem::UNKNOWN;//Info on the node's type
    bool isMutable=false;//Flag on the mutability of the node
    bool isConstant=false;//Flag on the node being constant at compile time
    int scopeDepth=-1;//Info on scope depth of the node
};

//Symbol that will be created per node and pushed to the symbol table
struct Symbol{
    std::string nodeName;
    TypeSystem nodeType;
    bool isMutable;
    bool isConstant;
    int scopeDepth;
};

//The semantic analyser class
class Semantics
{
    std::unordered_map<Node*, SemanticInfo> annotations;//Annotations map this will store the meta data per AST node
    std::vector<std::unordered_map<std::string,Symbol>> symbolTable;//This the symbol table which is a stack of hashmaps that will store info about the node during analysis 
    
    public: 
        Semantics(Node* node);//Semantics class analyzer
        void analyzer(Node* node);//The walker that will traverse the AST

        using analyzerFuncs=void (Semantics::*)(Node*);
        std::map<std::type_index,analyzerFuncs> analyzerFunctionsMap;

        //----------WALKER FUNCTIONS FOR DIFFERENT NODES---------
        void analyzeLetStatements(Node* node);
        void analyzeIntegerLiteral(Node* node);
        void analyzeFloatLiteral(Node* node);
        void analyzeStringLiteral(Node* node);
        void analyzeCharLiteral(Node* node);
        void analyzeBooleanLiteral(Node* node);


    private:
        //---------HELPER FUNCTIONS----------
        void registerAnalyzerFunctions();
        TypeSystem mapTypeStringToTypeSystem(const std::string& typeStr);
        TypeSystem inferExpressionType(Expression* expr);
        std::string TypeSystemString(TypeSystem type);

};