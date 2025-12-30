#include "calculator.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

typedef union
{
    struct CALCULATOR calculator_proc_1_arg;
} argument_t;

struct thread_data_t
{
    struct svc_req *rqstp;
    SVCXPRT *transp;
    argument_t argument;
    xdrproc_t xdr_argument;
    xdrproc_t xdr_result;
};
typedef struct thread_data_t thread_data_t;

static void *calculator_prog_worker(void *data_ptr)
{
    union {
        struct CALCULATOR calculator_proc_1_res;
    } result;
    thread_data_t *td = (thread_data_t *)data_ptr;
    
    // printf("Worker thread: %ld\n", pthread_self());
    
    if (calculator_proc_1_svc((struct CALCULATOR *)&td->argument, 
                              (struct CALCULATOR *)&result, td->rqstp) > 0) {
        if (!svc_sendreply(td->transp, td->xdr_result, (char *)&result))
            svcerr_systemerr(td->transp);
    }
    
    if (!svc_freeargs(td->transp, td->xdr_argument, (caddr_t)&td->argument))
        fprintf(stderr, "unable to free arguments");
    
    if (!calculator_prog_1_freeresult(td->transp, td->xdr_result, (caddr_t)&result))
        fprintf(stderr, "unable to free results");
    
    free(data_ptr);
    return NULL;
}

void calculator_create_thread(struct svc_req *rqstp, SVCXPRT *transp, 
                              xdrproc_t xdr_argument, xdrproc_t xdr_result)
{
    thread_data_t *thread_data_ptr = (thread_data_t *)malloc(sizeof(thread_data_t));
    if (thread_data_ptr == NULL) {
        perror("malloc");
        exit(1);
    }
    thread_data_ptr->rqstp = rqstp;
    thread_data_ptr->transp = transp;
    thread_data_ptr->xdr_argument = xdr_argument;
    thread_data_ptr->xdr_result = xdr_result;
    memset ((char *)&thread_data_ptr->argument, 0, sizeof (thread_data_ptr->argument));
    
    if (!svc_getargs (transp, xdr_argument, (caddr_t) &thread_data_ptr->argument)) {
        svcerr_decode (transp);
        free(thread_data_ptr);
        return;
    }
    
    pthread_attr_t attr;
    pthread_t thread;
    
    if (pthread_attr_init(&attr) == -1) {
        perror("pthread_attr_init");
        svc_freeargs (transp, xdr_argument, (caddr_t) &thread_data_ptr->argument);
        free(thread_data_ptr);
        exit(1);
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == -1) {
        perror("pthread_attr_setdetachstate");
        pthread_attr_destroy(&attr);
        svc_freeargs (transp, xdr_argument, (caddr_t) &thread_data_ptr->argument);
        free(thread_data_ptr);
        exit(1);
    }
    if (pthread_create(&thread, &attr, calculator_prog_worker, (void *)thread_data_ptr) == -1) {
        perror("pthread_create");
        pthread_attr_destroy(&attr);
        svc_freeargs (transp, xdr_argument, (caddr_t) &thread_data_ptr->argument);
        free(thread_data_ptr);
        exit(1);
    }
    pthread_attr_destroy(&attr);
}

bool_t calculator_proc_1_svc(struct CALCULATOR *argp, struct CALCULATOR *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;
	switch(argp->op) {
		case ADD:
			result->result = argp->arg1 + argp->arg2;
			break;
		case SUB:
			result->result = argp->arg1 - argp->arg2;
			break;
		case MUL:
			result->result = argp->arg1 * argp->arg2;
			break;
		case DIV:
			result->result = argp->arg1 / argp->arg2;
			break;
		default:
			retval = FALSE;
			break;
	}
	return retval;
}
int calculator_prog_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free(xdr_result, result);
	return 1;
}
