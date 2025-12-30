#include "calculator.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif

extern void calculator_create_thread(struct svc_req *rqstp, SVCXPRT *transp, 
                                     xdrproc_t xdr_argument, xdrproc_t xdr_result);

static void calculator_prog_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
    xdrproc_t _xdr_argument, _xdr_result;

    switch (rqstp->rq_proc) {
    case NULLPROC:
        (void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
        return;

    case CALCULATOR_PROC:
        _xdr_argument = (xdrproc_t) xdr_CALCULATOR;
        _xdr_result = (xdrproc_t) xdr_CALCULATOR;
        calculator_create_thread(rqstp, transp, _xdr_argument, _xdr_result);
        break;

    default:
        svcerr_noproc (transp);
        return;
    }
}

int main (int argc, char **argv)
{
    register SVCXPRT *transp;
    pmap_unset (CALCULATOR_PROG, CALCULATOR_VER);
    transp = svcudp_create(RPC_ANYSOCK);
    if (transp == NULL) {
        fprintf (stderr, "%s", "cannot create udp service.");
        exit(1);
    }
    if (!svc_register(transp, CALCULATOR_PROG, CALCULATOR_VER, calculator_prog_1, IPPROTO_UDP)) {
        fprintf (stderr, "%s", "unable to register (CALCULATOR_PROG, CALCULATOR_VER, udp).");
        exit(1);
    }
    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        fprintf (stderr, "%s", "cannot create tcp service.");
        exit(1);
    }
    if (!svc_register(transp, CALCULATOR_PROG, CALCULATOR_VER, calculator_prog_1, IPPROTO_TCP)) {
        fprintf (stderr, "%s", "unable to register (CALCULATOR_PROG, CALCULATOR_VER, tcp).");
        exit(1);
    }
    svc_run ();
    fprintf (stderr, "%s", "svc_run returned");
    exit (1);
}
