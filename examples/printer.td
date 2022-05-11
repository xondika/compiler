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

int main ()
{
    int a = 6255;
    printnumber (a);
    return 0;
}
