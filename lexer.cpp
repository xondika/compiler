#include "lexer.hpp"

#include <algorithm>

// std::string to_string( Token token ){
//     using enum Token;
//     switch( token ){
//         case Type:
//             return "Type";
//         case Keyword:
//             return "Keyword";
//     }
// }

void dictionary::add_word( std::string_view word, Token token, key k ){
    node* current = root.get();
    size_t wordIndex = 0;

    while( wordIndex < word.size() ){
        int childIndex = 0;
        bool found = false;
        for( auto& child : current->children ){
            if( child->c > word[ wordIndex ] ){
                current->children.insert( current->children.begin() + childIndex,
                                          std::make_unique< node >( word[ wordIndex ] ) );
                current = current->children[ childIndex ].get();
                ++wordIndex;
                found = true;
                break;
            }
            if( child->c == word[ wordIndex ] ){
                current = child.get();
                ++wordIndex;
                found = true;
                break;
            }
            ++childIndex;
        }
        if( !found ){
            current->children.emplace_back( std::make_unique< node >( word[ wordIndex ] ) );
        }
    }
    current->token = token;
    current->k = k;
}

void dictionary::remove_word( std::string_view word ){
    node* current = root.get();
    size_t wordIndex = 0;

    while( wordIndex < word.size() ){
        int childIndex = 0;
        bool found = false;
        for( auto& child : current->children ){
            if( child->c == word[ wordIndex ] ){
                if( wordIndex == word.size() - 1 ){
                    current->children.erase( current->children.begin() + childIndex );
                }
                current = child.get();
                ++wordIndex;
                found = true;
            }
            ++childIndex;
        }
        if( !found ){
            return;
        }

    }
}

Token dictionary::get_token( std::string_view word, key* k ){
    node* current = root.get();
    size_t wordIndex = 0;
    while( wordIndex < word.size() ){
        bool found = false;
        for( auto& child : current->children ){
            if( child->c == word[ wordIndex ] ){
                current = child.get();
                ++wordIndex;
                found = true;
            }
        }
        if( !found ){
            return None;
        }
    }
    if( k )
        *k = current->k;
    return current->token;
}

void dictionary::print_tokens( node* current, std::string str ){
    if( !current ){
        current = root.get();
    }
    if( current->token != None ){
        std::cout << str << ", " << (int) current->token << '\n';
    }
    for( auto& child : current->children ){
        print_tokens( child.get(), str + child->c );
    }
}

lexer::lexer(){
    init_tokens();
    init_types();
    init_keywords();
    init_operators();
}

void lexer::init_tokens(){
}

void lexer::init_types(){
    symbols.add_word( "int", Type, key( Types::Int ) );
}

void lexer::init_keywords(){
    symbols.add_word( "return", Keyword, key( Keywords::Return ) );
    symbols.add_word( "if", If, 0 );
}

void lexer::init_operators(){
    symbols.add_word( "+", Operator, key( Operators::Intplus ) );
    symbols.add_word( "=", Operator, key( Operators::Equals ) );
    symbols.add_word( "*", Operator, key( Operators::Intmul ) );
    symbols.add_word( "-", Operator, key( Operators::Intmin ) );
    symbols.add_word( "/", Operator, key( Operators::Intdiv ) );
}
