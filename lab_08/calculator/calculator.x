/*
 * filename: calculator.x
 * function: Define constants, non-standard data types and the calling process in remote calls
 */

struct CALCULATOR
{
    int op;        
    float arg1;    
    float arg2;    
    float result;
};

program CALCULATOR_PROG
{
    version CALCULATOR_VER
    {
        struct CALCULATOR CALCULATOR_PROC(struct CALCULATOR) = 1;
    } = 1; /* Версия интерфейса */
} = 0x20000001; /* Номер программы RPC */
