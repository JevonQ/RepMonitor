#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum file_type
{
    F_FILE = 0,
    F_DIRECTORY,
    F_SYMLINK,
    F_DEVICE,
    F_SPECIAL_FILE
} file_type_t;

typedef enum transfer_type
{
    T_TYPE_SENT = 0,
    T_TYPE_RECEIVED,
    T_TYPE_OTHER
} transfer_type_t;

typedef struct transfer_file
{
    char filename[256];
    file_type_t f_type;
    transfer_type_t t_type;
    int size;
    int pid;
    char last_line[100];
} transfer_file_t;

int process_logfile(char *, transfer_file_t *, char *);
void get_temp_filename(char *, char *, char *);
