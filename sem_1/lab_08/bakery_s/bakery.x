program BAKERY_PROG
{
    version BAKERY_VER
    {
        int GET_NUMBER(void) = 1;
        int BAKERY_SERVICE(int) = 2;
    } = 1; /* Version number = 1 */
} = 0x20000001; /* RPC program number */
