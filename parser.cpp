#include "parser.hpp"

#include <cassert>
#include <sstream>

void parser::parse( std::string path ){
    file = std::ifstream( path );
    if( !file ){
        throw std::invalid_argument("File not found\n");
    }

    root = std::make_unique< ast_node >( parse_root() );
}


ast_node parser::parse_root(){
    return parse_function();
}

ast_node parser::parse_function(){
    std::string word;
    function f;
    key typekey;

    file >> word;
    Token token = lex.symbols.get_token( word, &typekey );
    if( token == None ){
        throw std::invalid_argument("Invalid function type\n");
    }
    f.type = Types( typekey );
    file >> word;
    f.name = word;

    parse_args( f );

    lex.functions.push_back( f );

    ast_node f_node( Function, lex.functions.size() - 1 );

    eat_char( '{' );
    f_node.children = parse_expressions();
    eat_char( '}' );

    return f_node;
}

void parser::parse_args( function& f ){
    eat_char( '(' );
    //TODO: arguments
    // std::string word;

    eat_char( ')' );
}

std::vector< std::unique_ptr< ast_node > > parser::parse_expressions(){
    std::vector< std::unique_ptr< ast_node > > result;
    std::string expr;
    char c;
    while( ( c= file.peek() ) != '}' ){
        //std::cout << "wat\n";
        while( ( c = file.get() ) != ';' ){
            expr += c;
        }
        auto child = std::make_unique< ast_node >( parse_expr( expr ) );
        if( !child->children.empty() ){
            result.push_back( std::move( child ) );
        }
        expr = "";
        skipws();
    }

    return result;
}

std::string cut_spaces( std::string str ){
    size_t begin = 0;
    size_t end = str.size() - 1;

    while( std::isspace( str[ end ] ) ){
        --end;
    }
    while( std::isspace( str[ begin ] ) ){
        ++begin;
    }
    return std::string( str.begin() + begin, str.begin() + end + 1 );
}

ast_node parser::parse_expr( std::string str ){
    std::string word;
    std::cout << str << '\n';

    str = cut_spaces( str );
    if( std::all_of( str.begin(), str.end(), []( char c ){ return isdigit( c ); } ) )
     //|| std::all_of( str.begin() + 1, str.end(), []( char c ){ return isdigit( c ); } )
    {
        lex.integers.push_back( std::stoi( str ) );
        return { Literal, lex.integers.size() - 1 };
    }

    key k;
    if( lex.symbols.get_token( str, &k ) == Identifier ){
        return { Identifier, k };
    }

    std::stringstream ss( str );
    ast_node expr( Expression, 0 );
    ss >> word;
    Token token = lex.symbols.get_token( word, &k );
    if( token == Keyword ){
        //throw std::invalid_argument( std::string("Invalid expression: ") + word );

        expr.children.emplace_back( std::make_unique< ast_node >( token, k ) );

        std::string remainder;
        std::getline( ss, remainder, ';' );
        expr.children.emplace_back( std::make_unique< ast_node >( parse_expr( remainder ) ) );
    }
    else if( token == Type ){
        expr.children.emplace_back( std::make_unique< ast_node >( parse_declaration( str ) ) );
    }
    else {
        std::string left;
        while( token != Operator ){
            left += " " + word;
            if( !( ss >> word ) ){
                throw std::invalid_argument( "Expected operator, found none in: " + str );
            }
            token = lex.symbols.get_token( word, &k );
        }
        std::string right;
        std::getline( ss, right, ';' );
        expr.children.emplace_back( std::make_unique< ast_node >( token, k ) );
        expr.children.emplace_back( std::make_unique< ast_node >( parse_expr( left ) ) );
        expr.children.emplace_back( std::make_unique< ast_node >( parse_expr( right ) ) );
    }
    return expr;
}

ast_node parser::parse_declaration( std::string str ){
    //std::cout << str << '\n';
    std::stringstream ss( str );
    std::string word;
    ss >> word;
    key k;
    lex.symbols.get_token( word, &k );
    Types type = Types( k );
    ss >> word;
    lex.symbols.add_word( word, Identifier, lex.identifiers.size() );
    lex.identifiers.emplace_back( type, lex.functions.back().variables.size() );
    lex.functions.back().variables.push_back( type );
    return { Keyword, key( Keywords::Declaration ) };
}

void parser::eat_char( char expected ){
    skipws();
    char c = file.get();
    if( c != expected ){
        throw std::invalid_argument( std::string( "Missing '" ) + expected + "'\n" );
    }
    skipws();
}

void parser::skipws(){
    while( std::isspace( file.peek() ) ){
        file.get();
    }
}

void print( ast_node* current, std::string indent ){
    std::cout << indent << "Token: " << int( current->token ) <<
        " key: " << current->k << '\n';
    for( auto& child : current->children ){
        print( child.get(), indent + " " );
    }
}
void parser::print_ast(){
    print( root.get(), "" );
}

void parser::traverse(
    ast_node* current,
    std::map< key, std::vector< triple > >& functions,
    key index )
{
    if( current->token == Function ){
        for( auto& child : current->children ){
            traverse( child.get(), functions, current->k );
        }
    }
    if( current->token == Expression ){
        triple exp;
        if( current->children[ 0 ]->token == Keyword ){
            exp.keyword = Keywords( current->children[ 0 ]->k );
        } else {
            exp.op = Operators( current->children[ 0 ]->k );
        }

        auto traverse_child = [&]( size_t i ){
            if( current->children.size() > i ){
                if( current->children[ i ]->token == Expression ){
                    exp.arg1 = { Expression, functions[ index ].size() };
                    traverse( current->children[ i ].get(), functions, index );
                }
                auto& arg = i == 1 ? exp.arg1 : exp.arg2;
                arg = { current->children[ i ]->token,
                        current->children[ i ]->k };
            }
        };

        traverse_child( 1 );
        traverse_child( 2 );
        functions[ index ].push_back( exp );
    }
}

std::map< key, std::vector< triple > > parser::to_triples(){
    //print_ast();
    std::map< key, std::vector< triple > > functions;
    ast_node* current = root.get();
    traverse( current, functions, 0 );
    return functions;
}

std::string parser::to_instructions( triple t, std::string indent, key fkey ){
    std::string val1;
    std::string val2;

    if( t.keyword != Keywords::None ){
        using enum Keywords;
        switch( Keywords( t.keyword ) ){
            case Return:
                if( t.arg1.first == Token::Expression ){
                    return indent + "ret\n";
                }
                return indent + "mov " + to_instruction( t.arg1.first, t.arg1.second ) +
                       ", %eax\n" + indent + "add $" +
                       std::to_string( lex.functions[ fkey ].variables.size() * 4 ) +
                       ", %esp\n" +
                       indent + "ret\n";
            case Declaration:
                if( t.arg1.first != Token::None ){}
                return indent + "push $0\n";
            default:
                throw std::exception();
        }
    } else if( t.op != Operators::None ) {
        using enum Operators;
        switch( Operators( t.op ) ){
            case Intplus:
                ;
                val1 = std::to_string( lex.integers[ t.arg1.second ] );
                val2 = std::to_string( lex.integers[ t.arg2.second ] );

                return indent + "movl $" + val1 + ", %eax\n" + indent + "movl $" + val2
                    + ", %edx\n" + indent + "add %edx, %eax\n";

            case Equals:
                return indent + "movl " + to_instruction( t.arg2.first, t.arg2.second ) +
                    ", " + to_instruction( t.arg1.first, t.arg2.second ) + "\n";
        }
    }
    assert( false );
}

std::string parser::to_instruction( Token token, key k ){
    switch( token ){
        case Identifier:
            // if( k == 0 ){
            //     return "(%esp)";
            // }
            return std::to_string( k ) + "(%esp)";
        case Keyword:
            switch( Keywords( k ) ){
                case Keywords::Return:
                    return "ret";
                default:
                    return "";
            }
        case Literal:
            return std::string( "$" ) + std::to_string( lex.integers[ k ] );
        default:
            return "";
    }
}

void parser::translate( std::string path ){
    output_file = std::ofstream( path );
    std::string output;

    output = ".text\n    .global _start\n    .global _main\n";

    output += "_start:\n  call _main\n";
    // output += "  push $'\\n'\n  push $0x31\n  movl $4, %eax\n"
    //     "  movl $1, %ebx\n  mov %esp, %ecx\n  movl $5, %edx\n  int $0x80\n"
    //     "  movl $1, %eax\n  movl $0, %ebx\n  int $0x80\n";

    output += "  mov %eax, %ebx\n  mov $1, %eax\n  int $0x80\n";

    auto triples = to_triples();

    for( auto [ fkey, triplevec ] : triples ){
        output += "_" + lex.functions[ fkey ].name + ":\n";
        for( auto tri : triplevec ){
            output += to_instructions( tri, "  ", fkey );
        }
    }
    output_file << output;
    output_file.close();
}
