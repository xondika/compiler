#pragma once

#include "lexer.hpp"

#include <fstream>

/*Grammar:


Root = Function
Function = Type Identifier "(" ")" "{" Expression "}" ;
Type = int
Identifier = [a-z | A-Z]+
Expression = return [ Identifier | Value ] | Identifier = Value
Value = Int | Value + Value
Int = [0-9]+




*/

struct ast_node {
    Token token = Token::None;
    key k = 0;
    std::vector< std::unique_ptr< ast_node > > children;

    ast_node() = default;
    ast_node( Token t, key k ) : token( t ), k( k ) {}
};

struct triple{
    Keywords keyword = Keywords::None;
    Operators op = Operators::None;
    std::pair< Token, key > arg1 = { Token::None, 0 }, arg2 = { Token::None, 0 };
};

class parser {
    using enum Token;

    std::unique_ptr< ast_node > root;
    lexer lex;
    std::ifstream file;
    std::ofstream output_file;

  public:
    void parse( std::string path );

    void print_ast();

    void translate( std::string path );

    std::map< key, std::vector< triple > > to_triples();

  private:
    ast_node parse_root();
    ast_node parse_function();
    void parse_args( function& f );
    std::vector< std::unique_ptr< ast_node > > parse_expressions();
    ast_node parse_expr( std::string str );
    //ast_node parse_value( std::string str );
    void eat_char( char expected );
    void skipws();

    void traverse( ast_node* current,
                   std::map< key, std::vector< triple > >& functions,
                   key index = 0 );

    std::string to_instructions( triple t, std::string indent = "" );
    std::string to_instruction( Token token, key k );
};
