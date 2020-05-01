#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(2, "Usage: sleep seconds\n");
		exit();
	}
	
	int second = atoi(argv[1]);
	if (second < 0) {
		fprintf(2, "The number should not be negative.\n");
		exit();
	}
	
	if(sleep(second) < 0) {
		fprintf(2, "Sleep failed\n");
	}
	
	exit();
}


