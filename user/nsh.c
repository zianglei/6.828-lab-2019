#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#ifndef MAX_ONE_LINE_LEN
#define MAX_ONE_LINE_LEN 500
#endif

#ifndef MAX_ARGS
#define MAX_ARGS 30
#endif

#ifndef MAX_COMMAND_LEN
#define MAX_COMMAND_LEN 50
#endif

#ifndef MAX_COMMAND_NUM
#define MAX_COMMAND_NUM 10
#endif

#ifndef MAX_DIR_FILE_LEN
#define MAX_DIR_FILE_LEN 20
#endif

#define NULL 0

#define LOG(fmt, ...) \
	fprintf(2, "[%s] " fmt "\n", __func__, ## __VA_ARGS__)



char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";


struct cmd {
	int type;
	char* argv[MAX_ARGS];
	char* eargv[MAX_ARGS];

	char* infile;
	char* einfile;

	char* outfile;
	char* eoutfile;
};

typedef struct cmd cmd_s;

cmd_s cmd_list[MAX_COMMAND_NUM];

static int cmd_index = 0;

int peek(char** p, char* ep, char* token);
int
gettoken(char **ps, char *es, char **q, char **eq);



void panic(char* c) {
	fprintf(2, "%s\n", c);
	exit(-1);
}

int parseredirs(char** input, char* es, struct cmd* c) {
	
	int ret;
	char* start, *end;
	while (peek(input, es, "<>")) {
		ret = gettoken(input, es, 0, 0);
		if (ret == '<') {
			ret = gettoken(input, es, &start, &end);
			if (ret != 'a') {
				panic("parseredir stdin");
			}
			c->infile = start;
			c->einfile = end;
			*(c->einfile) = 0;
		} else if (ret == '>') {
			ret = gettoken(input, es, &start, &end);
			if (ret != 'a') {
				panic("parseredir stdout");
			}
			c->outfile = start;
			c->eoutfile = end;
			*(c->eoutfile) = 0;
		}
	}
	return 0;
}

struct cmd parsecmd(char** input, char* es) {
	
	int ret, args = 0;
	struct cmd c;
	memset(&c, 0, sizeof(struct cmd));
	char* start, *end;
	while(!peek(input, es, "|")) {
		ret = gettoken(input, es, &start, &end);
		if (ret == 0) break;
		// Need to fix
		if (ret != 'a') {
			exit(-1);
		}
		c.argv[args] = start;
		c.eargv[args] = end;
		args++;
		if (args >= MAX_ARGS) break;
		parseredirs(input, es, &c);
	}

	for(int i = 0; i <args; i++) {
		*(c.eargv[i]) = 0;
	}

	return c;
}

int parsepipe(char** input, char* es) {
	
	struct cmd c;
	c = parsecmd(input, es);
	cmd_list[cmd_index++] = c;
	if (peek(input, es, "|")) {
		gettoken(input, es, 0, 0);
		cmd_list[cmd_index++] = parsecmd(input, es);
	}
	return 0;
}


void
runcmd() {

	if (cmd_index == 1) {
		struct cmd c;
		c = cmd_list[0];
		if(fork() == 0) {
			if (c.infile != 0) {
				close(0);
				if(open(c.infile, O_RDONLY) < 0){
					panic("open redir stdin file");
				}
			}
			if (c.outfile != 0) {
				close(1);
				if (open(c.outfile, O_CREATE | O_WRONLY) < 0) {
					panic("open redir stdout file");
				}
			}
			exec(c.argv[0], c.argv);
		}
		wait(0);

	} else {
		// only support two commands
		int p[2]; pipe(p);
		struct cmd left = cmd_list[0];
		struct cmd right = cmd_list[1];

		if (fork() == 0) {
			if (left.infile) {
				close(0);
				if (open(left.infile, O_RDONLY) < 0) {
					panic("open left redir stdin file");
				}
			}
			close(p[0]);
			close(1);
			dup(p[1]);
			close(p[1]);
			exec(left.argv[0], left.argv);
		}
		if (fork() == 0) {
			if (right.outfile) {
				close(1);
				if (open(right.outfile, O_CREATE | O_WRONLY) < 0) {
					panic("open right redir stdin file");
				}
			}
			close(p[1]);
			close(0);
			dup(p[0]);
			close(p[0]);
			exec(right.argv[0], right.argv);
		}
		close(p[0]);
		close(p[1]);
		wait(0);
		wait(0);
	}
}


void clear_cmd_list() {
	cmd_index = 0;
}

int
getcmd(char* input, int size) {
	fprintf(2, "@ ");
	memset(input, 0, size);
	gets(input, size);
	if (input[0] == 0) return -1;
	return 0;
}

int main() {
	char input[MAX_ONE_LINE_LEN];
	while (getcmd(input, MAX_ONE_LINE_LEN) >= 0) {
		clear_cmd_list();
		char *s = input;
		if (fork() == 0)  {
			parsepipe(&s, input + strlen(input) + 1);
			runcmd();
		}
		wait(0);
	}
	exit(0);
}


// gettoken is copied from "user/sh.c"
int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

// peek is copied from "user/sh.c"
int peek(char** p, char* ep, char* token) {
	char *s;
	s = *p;
	while (s < ep && strchr(whitespace, *s))
		s++;
	*p = s;
	return *p && strchr(token, *s);
}


