/*
 * filename: calculator.x
 * function: Define constants, non-standard data types and the calling process in remote calls
 */

struct CALCULATOR
{
    int op;        
    double arg1;    
    double arg2;    
    double result; 
};

program CALCULATOR_PROG
{
    version CALCULATOR_VER
    {
        struct CALCULATOR CALCULATOR_PROC(struct CALCULATOR) = 1;
    } = 1; /* Версия интерфейса */
} = 0x20000001; /* Номер программы RPC */
