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

//int
//parseexec(char** input, char* es, struct cmd *c) {
//	int args = 0, ret = 0;
//	char *start, *end;
//	//while(*input < es && strchr(whitespace, **input)) (*input)++;
//	while (args < MAX_ARGS){
//		ret = gettoken(input, es, &start, &end);
//		LOG("gettoken ret: %c", ret);
//		if (ret != 'a') break;
//		c->argv[args] = start;
//		c->eargv[args] = end;
//		args++;
//	}
//	LOG("args: %d", args);
//	if (args == 0) return -1;
//
//	// get all arguments, set their end to '\0'
//	for(int i = 0; i < args; i++) {
//		*(c->eargv[i]) = '\0';
//	}
////	ret = gettoken(input, es, NULL, NULL);
//	LOG("next ret: %c", ret);
//	if (ret == '>' || ret == '<') {
//		// The command need redir
//		c->dir = ret == '<' ? 1: 2;
//		// get src or dst
//		char* dir_name_start, *dir_name_end;
//		ret = gettoken(input, es, &dir_name_start, &dir_name_end);
//		if (ret != 'a') return ret;
//		int length = (dir_name_end - dir_name_start) > MAX_DIR_FILE_LEN ? MAX_DIR_FILE_LEN : (dir_name_end - dir_name_start); 
//		LOG("file length: %d", length);	
//		strncpy(c->file, dir_name_start, length);
//		*(c->file + length) = 0;
//		return gettoken(input, es, NULL, NULL);
//	} else {
//		c->file[0] = 0;
//		return ret;
//	}
//	return ret;
//}


//int
//parsepipe(char** input, char* es) {
//	
//	struct cmd c;
//	int ret;
//	memset(&c, 0, sizeof(struct cmd));
//	ret = parseexec(input, es, &c);
//	cmd_list[cmd_index++] = c;
//	if(ret == '|') {
//		LOG("input now: %c", **input);
//		parsepipe(input, es);
//	}
//	return 0;
//}

//void
//parsecmd(char* input) {
//	char* s= input;
//	char* es = input + strlen(input);
//	parsepipe(&s, es);
//	LOG("parsecmd exit");
//}


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


//void
//execcmd(int i, int inp) {
//	struct cmd c = cmd_list[i];
//	LOG("cmd_cnt: %d", cmd_index);
//	LOG("cmd: %s", c.argv[0]);
//	LOG("cmd_index: %d", i);
//	
//	LOG("cmd_redir: %d", c.dir);
//	LOG("cmd_redir_file: %s, len: %d", c.file, strlen(c.file));
//	if (fork() == 0) {
//		if (i != 0) {
//			close(0);
//			LOG("close stdin");
//			dup(inp);
//		} else {
//			if (c.dir == 1) {
//				close(0);
//				LOG("redir stdin");
//				if(open(c.file, O_RDONLY) < 0) {
//					LOG("open %s failed", c.file);
//				}
//			}
//		}
//		close(inp);
//		int newp[2];
//		pipe(newp);
//
//		if (i != cmd_index-1) {
//			LOG("close stdout");
//			close(1);
//			LOG("dup output pipe");
//			dup(newp[1]);
//		} else {
//			if (c.dir == 2) {
//				LOG("redir stdout");
//				close(1);
//				if(open(c.file, O_CREATE | O_WRONLY) < 0) {
//					LOG("open %s failed", c.file);
//				}
//			}
//		}
//		close(newp[1]);
//	
//		if (i == cmd_index -1) {
//			// The last command
//			close(newp[0]);
//			LOG("exec %s", c.argv[0]);
//			exec(c.argv[0], c.argv);
//		} else {
//			LOG("fork next cmd");
//			execcmd(i+1, newp[0]);
//			exec(c.argv[0], c.argv);
//		}
//	}
//	close(inp);
//}
//
//void
//runcmd() {
//	int p[2];
//	LOG("cmd_cnt: %d", cmd_index);
//	pipe(p);
//	close(p[1]);
//	execcmd(0, p[0]);
//	int t = cmd_index;
//	while (t--) wait(0);
//	exit(0);
//}

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

// gettoken is copied from "user/sh.c"
// peek is copied from "user/sh.c"
int peek(char** p, char* ep, char* token) {
	char *s;
	s = *p;
	while (s < ep && strchr(whitespace, *s))
		s++;
	*p = s;
	return *p && strchr(token, *s);
}


