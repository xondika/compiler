#include "lexer.hpp"
#include "parser.hpp"
#include "dimcli/libs/dimcli/cli.h"

#include "unistd.h"



int main( int argc, char** argv ){
    Dim::Cli cli;

    auto& in = cli.opt< std::string >( "i input" ).desc( "path to input file" );

    auto& out = cli.opt< std::string >( "o output", "out" ).desc( "path to file" );

    bool keep_as;
    cli.opt( &keep_as, "a assembly", false ).desc( "keep the intermediary assembly file" );

    if (!cli.parse(argc, argv))
        return cli.printError( std::cerr );

    parser p;

    try {
        p.parse( *in );
        p.translate( *out + ".s" );
    } catch( std::invalid_argument& e ){
        return 1;
    }

    std::string as = "as " + *out + ".s -o " + *out + ".o --32";
    system( as.c_str() );

    std::string ld = "ld -m elf_i386 -s -o " + *out + " " + *out + ".o";
    system( ld.c_str() );

    if( !keep_as ){
        std::string to_remove = *out + ".s";
        std::remove( to_remove.c_str() );
    }
    std::string to_remove = *out + ".o";
    std::remove( to_remove.c_str() );
}
