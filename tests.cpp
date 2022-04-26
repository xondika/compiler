#include "lexer.hpp"
#include "parser.hpp"

#include <cassert>

int main(){


    dictionary dict;
    std::string i = "Int";

    dict.add_word( i, Token::Type );

    assert( dict.get_token( i ) == Token::Type );
    assert( dict.get_token( "int" ) == Token::None );

    dict.add_word( "Ind", Token::Operator );
    assert( dict.get_token( "Ind" ) == Token::Operator );

    lexer lex;
//    lex.print_tokens();

    parser p;
    p.parse( "test.td" );
    p.print_ast();

    auto triples = p.to_triples();
    for( auto [ func, trips ] : triples ){
        std::cout << "Function: " << func << '\n';
        for( auto t : trips ){
            std::cout << " Keyword: " << int( t.keyword ) << " Op: " << int( t.op ) << '\n'
                << " token1: " << int( t.arg1.first ) << '\n';
        }
    }
    p.translate( "out.s" );
}
