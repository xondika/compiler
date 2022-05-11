int printrec ( int n )
{
    if ( n ) {
       printrec ( n / 10 );
       print ( n - ( 10 * (n / 10) ) ) + 48;
    }
    return 0;
}

int printnumber ( int number )
{
    printrec ( number );
    print 10;
    return 0;
}

int fib ( int a )
{
    if( a ){
        return a * fib ( a - 1 );
    }
    return 1;
}

int main ()
{
    int result = fib ( 10 );
    printnumber ( result );
    return 0;
}
