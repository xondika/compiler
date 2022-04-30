#pragma once

#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

enum class Token {
    None,
    Type,
    Keyword,
    Operator,
    Identifier,
    Literal,
    Expression,
    Function,
    Semicolon,
    Popen,
    Pclose,
    Copen,
    Cclose
};

enum class Types {
    None,
    Int
};

enum class Keywords {
    None,
    Return,
    Declaration
};

enum class Operators {
    None,
    Intplus,
    Equals
};

//std::map< Token, std::map< std::string,  > >

using key = uint64_t;

struct node {
    char c = 0;
    std::vector< std::unique_ptr< node > > children;
    Token token = Token::None;
    key k = 0;


    node() = default;
    node( char c ) : c( c ) {};

};

class dictionary {
    using enum Token;

    std::unique_ptr< node > root;

  public:

    dictionary() : root( std::make_unique< node >( 0 ) ) {}

    void add_word( std::string_view word, Token token, key k = 0 );

    void remove_word( std::string_view word );

    Token get_token( std::string_view word, key* k = nullptr );

    void print_tokens( node* current = nullptr, std::string str = "" );
};

struct function {
    std::string name;
    Types type;
    std::vector< Types > variables;
};

class parser;

class lexer {
    using enum Token;

    dictionary symbols;

    std::vector< Types > types;
    std::vector< Keywords > keywords;
    std::vector< Operators > operators;

    std::vector< std::pair< Types, key > > identifiers;
    std::vector< int64_t > integers;
    std::vector< function > functions;

    friend parser;

  public:
    lexer();

    void init_tokens();
    void init_types();
    void init_operators();
    void init_keywords();

    void print_tokens(){
        symbols.print_tokens();
    }
};
