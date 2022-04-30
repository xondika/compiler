#include "lexer.hpp"
#include "parser.hpp"
#include "unistd.h"

int main(){
    parser p;
    p.parse( "test.td" );
    p.translate( "out.s" );

    system("as out.s -o out.o --32");
    system("ld -m elf_i386 -s -o out out.o");
}
