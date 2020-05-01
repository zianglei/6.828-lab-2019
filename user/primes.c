#include "kernel/types.h"
#include "user/user.h"

void subprime(int input) {

	int divide, temp;
	int p[2];

	if(read(input, &divide, sizeof(int)) <= 0) exit();
	printf("prime %d\n", divide);
	
	if (pipe(p) < 0) {
		fprintf(2, "pipe\n");
		exit();
	}

	if(fork() == 0) {
		close(p[1]);
		subprime(p[0]);
	} else {
		close(p[0]);
		while(read(input, &temp, sizeof(int)) >0) {
			if (temp % divide != 0) {
				write(p[1], &temp, sizeof(int));
			}
		}
		close(p[1]);
		close(input);
		wait();
		exit();
	}
}

int
main(int argc, char* argv[]) {

	int p[2];

	if (pipe(p) < 0) {
		fprintf(2, "pipe failed\n");
		exit();
	}
	
	if (fork() == 0) {
		close(p[1]);
		subprime(p[0]);
	} else {
		close(p[0]);
		for(int i = 2; i < 32; i++) {
			write(p[1], &i, sizeof(int));
		}
		close(p[1]);
		wait();
		exit();
	}
	return 0;

}
