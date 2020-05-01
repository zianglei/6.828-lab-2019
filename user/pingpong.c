#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char* argv[]) {
	
	int parent_fd[2], child_fd[2];
	if (pipe(parent_fd) < 0) {
		fprintf(2, "parent pipe\n");
		exit();
	}
	if (pipe(child_fd) < 0) {
		fprintf(2 , "child pipe\n");
		exit();
	}
	
	char c = 'a';

	
	close(parent_fd[0]);
	write(parent_fd[1], &c, sizeof(c));
	close(parent_fd[1]);

	if (fork() == 0) {
		// child
		close(parent_fd[1]);
		read(parent_fd[0], &c, sizeof(c));
		printf("%d: received ping\n", getpid());
		close(parent_fd[0]);

		close(child_fd[0]);
		write(child_fd[1], &c, sizeof(c));
		close(child_fd[1]);
		exit();
	}

	close(child_fd[1]);
	read(child_fd[0], &c, sizeof(c));
	printf("%d: received pong\n", getpid());
	close(child_fd[0]);

	wait();
	exit();
}
