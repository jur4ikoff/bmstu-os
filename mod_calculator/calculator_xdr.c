#include "calculator.h"

bool_t xdr_CALCULATOR(xdrs, objp)
	XDR *xdrs;
	CALCULATOR *objp;
{

	if (!xdr_int(xdrs, &objp->op))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->arg1))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->arg2))
		return (FALSE);
	if (!xdr_float(xdrs, &objp->result))
		return (FALSE);
	return (TRUE);
}
