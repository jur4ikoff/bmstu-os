#include "calculator.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
int flag = 1;
void sig_handler(int sig_num)
{
	flag = 0;
	printf("catch sig. pid: %d\n", getpid());
}
void calculator_prog_1(CLIENT *clnt)
{
	enum clnt_stat retval_1;
	struct CALCULATOR result_1;
	struct CALCULATOR  calculator_proc_1_arg;
	char sign;
	calculator_proc_1_arg.op = rand() % 4;
	calculator_proc_1_arg.arg1 = rand() % 64;
	calculator_proc_1_arg.arg2 = rand() % 64;
	retval_1 = calculator_proc_1(&calculator_proc_1_arg, &result_1, clnt);
	if (retval_1 != RPC_SUCCESS) {
		clnt_perror (clnt, "call failed");
		exit(1);
	}
	switch (calculator_proc_1_arg.op) {
		case ADD:
			sign = '+';
			break;
		case SUB:
			sign = '-';
			break;
		case MUL:
			sign = '*';
			break;
		case DIV:
			sign = '/';
			break;
		default:
			break;
	}
	printf("%f %c %f = %f\n", calculator_proc_1_arg.arg1, sign, calculator_proc_1_arg.arg2, result_1.result);
	usleep(1000);
}

int main (int argc, char *argv[])
{
	char *host;
	srand(getpid());
	CLIENT *clnt;
	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}
	host = argv[1];
	clnt = clnt_create (host, CALCULATOR_PROG, CALCULATOR_VER, "tcp");
	if (clnt == NULL) {
		printf("Failed to create client\n");
		clnt_pcreateerror (host);
		exit (1);
	}
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		perror("signal");
		printf("errno: %d\n", errno);
		exit(1);
	}
	while (flag) {
		calculator_prog_1(clnt);
	}
	clnt_destroy (clnt);
    exit(0);
}
