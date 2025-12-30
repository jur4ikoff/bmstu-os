#include <memory.h> 
#include "calculator.h"
static struct timeval TIMEOUT = { 25, 0 };
enum clnt_stat calculator_proc_1(struct CALCULATOR *argp, struct CALCULATOR *clnt_res, CLIENT *clnt)
{
    return (clnt_call(clnt, CALCULATOR_PROC,
        (xdrproc_t) xdr_CALCULATOR, (caddr_t) argp,
        (xdrproc_t) xdr_CALCULATOR, (caddr_t) clnt_res,
        TIMEOUT));
}
