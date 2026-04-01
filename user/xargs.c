#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int 
parse_buf(char *buf, char* tok[], int max) {
    int token_count = 0;
    char *p = buf;
    
    while (*p && token_count < max) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        
        if (*p == '\0') {
            break;
        }
        
        tok[token_count++] = p;
      
        while (*p && *p != ' ' && *p != '\t' && *p != '\n') {
            p++;
        }
        
        if (*p) {
            *p = '\0';
            p++;
        }
    }
    
    return token_count;
}




int
main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(2, "Usage: xargs <command> [arguments...]\n");
        exit(1);
    }

    char buf[512];    
    char *p = buf;

    while (read(0, p, 1) == 1) {
        if (p - buf >= sizeof(buf) - 1) {
            break;
        }
        if (*p == '\n') {
            *p = 0;
            if (fork() == 0) {
                int j = 0;
                char *args[MAXARG];
                char *args_line[MAXARG];
                for (int i = 0; i < argc - 1; i ++) {
                    args[j++] = argv[i+1];
                }
                int argc_line = parse_buf(buf, args_line, MAXARG - argc + 1);
                int i = 0;
                while (j < MAXARG - 1 && i < argc_line) {
                    args[j++] = args_line[i++];
                }
                args[j] = 0;
                exec(argv[1], args);
            } else {
                wait((int *) 0);
                p = buf;
            }
        } else {
            p ++;
        }
    }

    exit(0);
}