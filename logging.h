#ifndef LOGGING_H
#define LOGGING_H

void print_log_level(int level);
extern FILE *fptr;

#define log_message(t, x, ...) { print_log_level(t);\
                            fprintf(fptr," %s %s() line %d ",__FILE__,__func__,__LINE__);	\
							fprintf(fptr, x);											\
							fprintf(fptr, "\n");}


enum output {
    STDOUT,
    FILES
};

enum criticity {
    DEBUG,
    LOG,
    ERROR
};

int log_init(enum output out_flow, char * filename);

#endif /* LOGGING_H */