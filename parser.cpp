#include "parser.hpp"

#include <cassert>
#include <sstream>

void parser::parse( std::string path ){
    file = std::ifstream( path );
    if( !file ){
        error( "File not found\n" );
    }

    root = std::make_unique< ast_node >( parse_root() );
}


ast_node parser::parse_root(){
    ast_node root = { Root, 0 };
    while( file.peek() != EOF ){
        skipws();
        root.children.emplace_back( std::make_unique< ast_node >( parse_function() ) );
    }
    return root;
}

ast_node parser::parse_function(){
    std::string word;
    function f;
    key typekey;

    file >> word;
    Token token = lex.symbols.get_token( word, &typekey );
    if( token == None ){
        error("Invalid function type\n");
    }
    f.type = Types( typekey );
    file >> word;
    f.name = word;

    parse_args( f );

    lex.functions.push_back( f );

    ast_node f_node( Function, lex.functions.size() - 1 );
    lex.symbols.add_word( f.name, Function, lex.functions.size() - 1 );

    eat_char( '{' );
    f_node.children = parse_expressions();
    eat_char( '}' );

    return f_node;
}

void parser::parse_args( function& f, bool definition ){
    eat_char( '(' );
    while( file.peek() != ')' ){
        std::string word;
        file >> word;
        key k;
        Token token = lex.symbols.get_token( word, &k );
        if( token != Type ){
            error( word + "does not name a type." );
        }
        skipws();
        char c = file.peek();
        std::string arg;
        while( c != ')' && c != ',' && !isspace( c ) ){
            arg += c;
            file.get();
            c = file.peek();
        }
        if( c == ',' ){
            file.get();
        }
        lex.symbols.add_word( arg, Argument, f.arguments.size() );
        //f.push_back( Types( k ) );
        f.arguments.push_back( Types( k ) );

    }

    eat_char( ')' );
}

std::vector< std::unique_ptr< ast_node > > parser::parse_expressions(){
    std::vector< std::unique_ptr< ast_node > > result;
    std::string expr;
    char c;
    while( ( c = file.peek() ) != '}' ){
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

    while( std::isspace( str[ end ] ) // || str[ end ] == ')'
           ){
        --end;
    }
    while( std::isspace( str[ begin ] ) // || str[ begin ] == '('
           ){
        ++begin;
    }

    return std::string( str.begin() + begin, str.begin() + end + 1 );
}

std::string remove_brackets( std::string str ){
    int counter = 0;
    if( str.front() != '(' ){
        return str;
    }
    size_t pos = 0;
    for( char c : str ){
        if( c == '(' ){
            ++counter;
        }
        if( c == ')' ){
            if( --counter == 0 && pos != str.size() - 1 ){
                return str;
            }
        }
        pos++;
    }
    return remove_brackets( { str.begin() + 1, str.end() - 1 } );
}
ast_node parser::parse_expr( std::string str ){
    std::string word;
    if( !str.empty() ){
        std::cout << str << '\n';
    }


    str = cut_spaces( str );
    str = remove_brackets( str );
    if( str.empty() ){
        return { None, 0 };
    }
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
    if( lex.symbols.get_token( str, &k ) == Argument ){
        return { Argument, k };
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
        std::string remainder;
        std::getline( ss, remainder, ';' );
        auto assign = parse_expr( remainder );
        //expr.children.push_back( std::make_unique< ast_node >( std::move( assign ) ) );
        if( assign.children.front()->token == Operator
         && assign.children.front()->k == key( Operators::Equals ) )
        {
            expr.children.emplace_back( std::make_unique< ast_node >( std::move( *assign.children[ 2 ] ) ) );
        }
    }
    else if( token == Function ){
        expr.children.emplace_back( std::make_unique< ast_node >( Operator, key( Operators::Call ) ) );
        expr.children.emplace_back( std::make_unique< ast_node >( token, k ) );

        std::string argument;
        size_t index;
        while( isspace( ss.peek() ) ){
            ss.get();
        }
        if( ss.get() != '(' ){
            error( "Expecting brackets in function call: " + str );
        }
        size_t brackets = 1;
        char c = ss.peek();
        while( brackets != 0 ){
            while( c != ',' )
            {
                if( c == '(' ){
                    brackets++;
                }
                if( c == ')' ){
                    if( --brackets == 0 ){
                        ss.get();
                        break;
                    };
                }
                argument += c;
                ss.get();
                c = ss.peek();
            }
            expr.children.emplace_back( std::make_unique< ast_node >( parse_expr( argument ) ) );
            argument = "";
            while( isspace( c ) || c == ',' ){
                ss.get();
                c = ss.peek();
            }
        }

        std::string remainder;
        std::getline( ss, remainder, ';' );
        expr.children.emplace_back( std::make_unique< ast_node >( parse_expr( remainder ) ) );
    }
    else {
        std::string left;
        int brackets = 0;
        while( token != Operator || brackets != 0 ){
            left += " " + word;
            for( char c : word ){
                if( c == '(' ){
                    brackets++;
                }
                if( c == ')' ){
                    brackets--;
                }
            }
            if( !( ss >> word ) ){
                error( "Expected operator, found none in: " + str );
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

// ast_node parser::parse_callargs( std::string str ){

// }

ast_node parser::parse_declaration( std::string str ){
    //std::cout << str << '\n';
    std::stringstream ss( str );
    std::string word;
    ss >> word;
    key k;
    lex.symbols.get_token( word, &k );
    Types type = Types( k );
    ss >> word;
    lex.symbols.add_word( word, Identifier, lex.functions.back().variables.size() );
    lex.identifiers.emplace_back( type, lex.functions.back().variables.size() );
    lex.functions.back().variables.push_back( type );
    return { Keyword, key( Keywords::Declaration ) };
}

void parser::eat_char( char expected ){
    skipws();
    char c = file.get();
    if( c != expected ){
        error( std::string( "Missing '" ) + expected + "'\n" );
    }
    skipws();
}

void parser::skipws(){
    while( std::isspace( file.peek() ) ){
        if( file.peek() == '\n' ){
            ++line;
        }
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

        for( int i = 1; i < current->children.size(); ++i ){
            if( current->children[ i ]->token == Expression ){
                exp.args.emplace_back( Expression, functions[ index ].size() );
                traverse( current->children[ i ].get(), functions, index );
            } else {
                // std::cout << "token: " << key( current->children[ i ]->token ) <<
                //     " key: " << current->children[ i ]->k << '\n';
                exp.args.emplace_back( current->children[ i ]->token,
                                       current->children[ i ]->k );
            }
        }

        functions[ index ].push_back( exp );
    }
}

std::map< key, std::vector< triple > > parser::to_triples(){
    print_ast();
    std::map< key, std::vector< triple > > functions;
    //ast_node* current = root.get();
    for( auto& child : root->children ){
        traverse( child.get(), functions, 0 );
    }
    return functions;
}

std::string parser::arithmetic( triple t, std::string op, std::string s1, std::string s2,
                                std::string indent ){
    std::string result;

    if( t.args[ 0 ].first == Token::Expression
     && t.args[ 1 ].first == Token::Expression )
    {
        return indent + "mov %eax, %edx\n" + indent + "pop %eax\n"
            + indent + op + " %edx, %eax\n";
    }
    // TODO: if result is used again, push it
    // if( eax_full ){
    //     result += indent + "push %eax\n";
    // }
    // eax_full = true;
    if( t.args[ 0 ].first == Token::Expression ){
        return result + indent + op + " " + s2
            + ", %eax\n";
    }
    if( t.args[ 1 ].first == Token::Expression ){
        result += indent + "mov " + s1 + ", %edx\n";
        return result + indent + op + " "
            + "%edx, %eax\n";
    }

    return result + indent + "mov " + s1 + ", %eax\n" + indent +
        "mov " + s2
        + ", %edx\n" + indent + op + " %edx, %eax\n";

}

std::string parser::to_instructions( triple t, std::string indent, key fkey ){
//    std::cout << key( t.keyword ) << "op: " << key( t.op ) << " size: " << t.args.size() << '\n';
    std::string s1 = t.args.size() == 0 ? "" :
        to_instruction( t.args[ 0 ].first, t.args[ 0 ].second, fkey );
    std::string s2 = t.args.size() < 1 ? "" :
        to_instruction( t.args[ 1 ].first, t.args[ 1 ].second, fkey );

    std::string result;
    size_t pop = 0;
    if( t.keyword != Keywords::None ){
        using enum Keywords;
        switch( Keywords( t.keyword ) ){
            case Return:
                if( t.args[ 0 ].first == Token::Expression ){
                    return indent + "ret\n";
                }
                return indent + "mov " + s1 +
                       ", %eax\n" + // indent + "add $" +
                       // std::to_string( lex.functions[ fkey ].variables.size() * 4 ) +
                       // ", %esp\n" +
                       indent + "ret\n";
            case Declaration:
                if( t.args[ 0 ].first != Token::None ){
                    return indent + "push " + s1
                        + "\n";
                }
                return indent + "push $0\n";
            default:
                throw std::exception();
        }
    } else if( t.op != Operators::None ) {
        using enum Operators;
        switch( Operators( t.op ) ){
            case Intplus:
                return arithmetic( t, "add", s1, s2, indent );
            case Intmin:
                return arithmetic( t, "sub", s1, s2, indent );
            case Intdiv:
                return arithmetic( t, "idiv", s1, s2, indent );
            case Intmul:
                return arithmetic( t, "imul", s1, s2, indent );
            case Equals:
                if( t.args[ 1 ].first == Expression ){
                    return indent + "mov %eax, " + s1 + '\n';
                }
                return indent + "movl " + s2 +
                    ", " + s1 + "\n";
            case Call:
                for( size_t i = t.args.size() - 1; i > 0; --i ){
                    if( t.args[ i ].first != Token::None ){
                        result += indent + "push "
                            + to_instruction( t.args[ i ].first, t.args[ i ].second, fkey )
                            + "\n";
                        pop += 4;
                    }
                }
                return result + indent + "call " + s1 + '\n' + indent + "add $"
                    + std::to_string( pop ) + ", %esp\n";
        }
    }
    assert( false );
}

std::string parser::to_instruction( Token token, key k, key fkey ){
    switch( token ){
        case Argument:
            return std::to_string( 4 * ( lex.functions[ fkey ].variables.size() + 1 + k ) )
                + "(%esp)";
        case Identifier:
            // if( k == 0 ){
            //     return "(%esp)";
            // }
            return std::to_string( 4 * ( lex.functions[ fkey ].variables.size() - 1 - k ) )
                + "(%esp)";
        case Keyword:
            switch( Keywords( k ) ){
                case Keywords::Return:
                    return "ret";
                default:
                    return "";
            }
        case Literal:
            return std::string( "$" ) + std::to_string( lex.integers[ k ] );
        case Function:
            return "_" + lex.functions[ k ].name;
        default:
            return "";
    }
}

void parser::translate( std::string path ){
    output_file = std::ofstream( path );
    std::string output;

    std::string header = ".text\n    .global _start\n";

    std::string init = "_start:\n  call _main\n";
    // output += "  push $'\\n'\n  push $0x31\n  movl $4, %eax\n"
    //     "  movl $1, %ebx\n  mov %esp, %ecx\n  movl $5, %edx\n  int $0x80\n"
    //     "  movl $1, %eax\n  movl $0, %ebx\n  int $0x80\n";

    init += "  mov %eax, %ebx\n  mov $1, %eax\n  int $0x80\n\n";

    auto triples = to_triples();

    std::map< std::string, std::string > f_codes;
    for( auto [ fkey, triplevec ] : triples ){
        eax_full = false;
        header += "    .global _" + lex.functions[ fkey ].name + "\n";
        //output += "_" + lex.functions[ fkey ].name + ":\n";
        for( auto tri : triplevec ){
            output += to_instructions( tri, "  ", fkey );
        }
        f_codes[ "_" + lex.functions[ fkey ].name ] = output;
        output = "";
    }
    output_file << header << '\n' << init;
    for( auto [ f, inst ] : f_codes ){
        output_file << f << ":\n";
        output_file << inst << '\n';
    }
    //output_file << header << '\n' << output;
    output_file.close();
}

void parser::error( std::string str ){
    std::cerr << ( "Error on line: " + std::to_string( line ) + ":\n  " + str + "\n" );
    throw std::invalid_argument( "wat" );
}
