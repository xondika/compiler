#include "parser.hpp"

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
    while( ( c = file.get() ) != ';' ){
        expr += c;
    }
    result.emplace_back( std::make_unique< ast_node >( parse_expr( expr ) ) );

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

    str = cut_spaces( str );
    if( std::all_of( str.begin(), str.end(), []( char c ){ return isdigit( c ); } )
     || ( std::all_of( str.begin() + 1, str.end(), []( char c ){ return isdigit( c ); } )
       && str.front() == '-' ) )
    {
        lex.integers.push_back( std::stoi( str ) );
        return { Literal, lex.integers.size() - 1 };
    }

    std::stringstream ss( str );
    ast_node expr( Expression, 0 );
    ss >> word;
    Token token = lex.symbols.get_token( word );
    if( token != Keyword ){
        throw std::invalid_argument( std::string("Invalid keyword: ") + word );
    }
    expr.children.emplace_back( std::make_unique< ast_node >( token, 0 ) );

    std::string remainder;
    std::getline( ss, remainder, ';' );
    expr.children.emplace_back( std::make_unique< ast_node >( parse_expr( remainder ) ) );

    return expr;
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
        if( current->children.size() > 1 ){
            exp.arg1 = { current->children[ 1 ]->token,
                         current->children[ 1 ]->k };
        }
        if( current->children.size() > 2 ){
            exp.arg2 = { current->children[ 2 ]->token,
                         current->children[ 2 ]->k };
        }
        functions[ index ].push_back( exp );
    }
}

std::map< key, std::vector< triple > > parser::to_triples(){
    std::map< key, std::vector< triple > > functions;
    ast_node* current = root.get();
    traverse( current, functions, 0 );
    return functions;
}

std::string parser::to_instructions( triple t, std::string indent ){
    if( t.keyword != Keywords::None ){
        using enum Keywords;
        switch( Keywords( t.keyword ) ){
            case Return:
                return indent + "movl " + to_instruction( t.arg1.first, t.arg1.second ) +
                       ", %eax\n" + indent + "ret\n";
            default:
                throw std::exception();
        }
    }
}

std::string parser::to_instruction( Token token, key k ){
    switch( token ){
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

    output += "_start:\n  call _main\n  mov %eax, %ebx\n  movl $1, %eax\n  int $0x80\n";

    auto triples = to_triples();

    for( auto [ fkey, triplevec ] : triples ){
        output += "_" + lex.functions[ fkey ].name + ":\n";
        for( auto tri : triplevec ){
            output += to_instructions( tri, "  " );
        }
    }
    output_file << output;
}
