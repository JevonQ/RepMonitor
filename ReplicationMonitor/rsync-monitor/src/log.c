/*
 * This file includes the many functions which process the rsync log file
 * I need get the following details:
 * * filesTransferred *
 * * filesExamined *
 * * bytesSent *
 * * totalTime *
 *
 *
 * YXcstpoguax  path/to/file
 * |||||||||||
 * `----------- the type of update being done::
 *  ||||||||||   <: file is being transferred to the remote host (sent).
 *  ||||||||||   >: file is being transferred to the local host (received).
 *  ||||||||||   c: local change/creation for the item, such as:
 *  ||||||||||      - the creation of a directory
 *  ||||||||||      - the changing of a symlink,
 *  ||||||||||      - etc.
 *  ||||||||||   h: the item is a hard link to another item (requires --hard-links).
 *  ||||||||||   .: the item is not being updated (though it might have attributes that are being modified).
 *  ||||||||||   *: means that the rest of the itemized-output area contains a message (e.g. "deleting").
 *  ||||||||||
 *  `---------- the file type:
 *   |||||||||   f for a file,
 *   |||||||||   d for a directory,
 *   |||||||||   L for a symlink,
 *   |||||||||   D for a device,
 *   |||||||||   S for a special file (e.g. named sockets and fifos).
 *   |||||||||
 *   `--------- c: different checksum (for regular files)
 *    ||||||||     changed value (for symlink, device, and special file)
 *    `-------- s: Size is different
 *     `------- t: Modification time is different
 *      `------ p: Permission are different
 *       `----- o: Owner is different
 *        `---- g: Group is different
 *         `--- u: The u slot is reserved for future use
 *          `-- a: The ACL information changed
 */

#include "log.h"

int
process_logfile(char *filename, transfer_file_t *file_list, char *last_line)
{
    FILE *fd;
    char line[100];
    char * find_p;
    char * p;
    int year, month, day, hour, min, sec, pid, size;
    char temp[12];
    char name[256];
    int num = 0;
    int message = 0;

    strcpy(file_list[0].filename, "message");

    if ((fd = fopen(filename, "rt")) == NULL) {
        printf("Can not open file : %s\n", filename);
        return -1;
    }
    while (fgets(line, 100, fd) != NULL) {
        if ((find_p = strstr(line, "building file list")) != NULL) {
            //printf("build\n");
            fgets(line, 100, fd); // get the next line after 'building file list'
            if ((find_p = strstr(line, "sent ")) != NULL && (find_p = strstr(line, "total size")) != NULL) {
                sscanf(line, "%d/%d/%d %d:%d:%d [%d] sent %d ", &year, &month, &day, &hour, &min, &sec, &pid, &size);
                //printf("File transfered: %d\n", size);
                file_list[0].size += size;
                strcpy(file_list[0].last_line, line);
                strcpy(last_line, line);
                file_list[0].pid = pid;
                continue;
            }
            if ((find_p = strstr(line, "Number of files:")) != NULL) {
                message = 0;
                continue;
            }

            sscanf(line, "%d/%d/%d %d:%d:%d [%d] %11s %s", &year, &month, &day, &hour, &min, &sec, &pid, temp, name);
            if (temp[0] != '<' && temp[0] != '>' && temp[0] != 'c' && temp[0] != 'h' && temp[0] != '.' && temp[0] != '*') {
                continue;
            }
            if (temp[1] != 'd' && temp[1] != 'f' && temp[1] != 'L' && temp[1] != 'D' && temp[1] != 'S') {
                continue;
            }
            message = ++num;
            strcpy(file_list[message].filename, name);
            file_list[message].pid = pid;
            if (temp[0] == '>') {
                file_list[message].t_type = T_TYPE_RECEIVED;
            } else if (temp[0] == '<') {
                file_list[message].t_type = T_TYPE_SENT; 
            } else {
                file_list[message].t_type = T_TYPE_OTHER;
            }
            if (temp[1] == 'f') {
                file_list[message].f_type = F_FILE;
            } else if (temp[1] == 'd') {
                file_list[message].f_type = F_DIRECTORY;
            } else if (temp[1] == 'L') {
                file_list[message].f_type = F_SYMLINK;
            } else if (temp[1] == 'D') {
                file_list[message].f_type = F_DEVICE;
            } else { // temp[1] == 'S' 
                file_list[message].f_type = F_SPECIAL_FILE;
            }
            //printf("%s - %s\n", temp, name);
        } else if ((find_p = strstr(line, "sent ")) != NULL && (find_p = strstr(line, "total size")) != NULL) {
            sscanf(line, "%d/%d/%d %d:%d:%d [%d] sent %d ", &year, &month, &day, &hour, &min, &sec, &pid, &size);
            //printf(" The transfer bytes: %d\n", size);
            strcpy(file_list[message].last_line, line);
            strcpy(last_line, line);
            file_list[message].size += size;
        }
    }
    fclose(fd);

    return num;
}

void
get_temp_filename(char *filename, char *temp_filename, char *last_line)
{
    FILE *fd1, *fd2;
    char line[100];
    char temp_line[100];
    char cmd[100];
    char *find_p;
    int write = 0;
    int year, month, day, hour, min, sec, pid1, pid2, size1, size2;

    memset(line, '\0', 100);
    fd1 = fopen(filename, "rt");
    if (fd1 == NULL) {
        printf("Can not open file %s\n", filename);
        return;
    }

    fd2 = fopen(temp_filename, "wt");
    if (fd2 == NULL) {
        printf("Can not write file %s\n", temp_filename);
        return;
    }

    while(fgets(line, 100, fd1) != NULL) {
        if ((write == 0) && (!strncmp(line, last_line, 20))) {
            if (!((find_p = strstr(line, "sent ")) != NULL && (find_p = strstr(line, "total size")) != NULL)) {
                continue;
            }

            sscanf(line, "%d/%d/%d %d:%d:%d [%d] sent %d ", &year, &month, &day, &hour, &min, &sec, &pid1, &size1);
            sscanf(last_line, "%d/%d/%d %d:%d:%d [%d] sent %d ", &year, &month, &day, &hour, &min, &sec, &pid2, &size2);
            if (pid1 == pid2 && size1 == size2) {
                write = 1;
                continue;
            } else if (pid1 == pid2 && size1 > size2) {
                memset(temp_line, '\0', 100);
                sprintf(temp_line, "%d/%d/%d %d:%d:%d [%d] building file list\n", year, month, day, hour, min, sec, pid1);
                fputs(temp_line, fd2);
                memset(temp_line, '\0', 100);
                sprintf(temp_line, "%d/%d/%d %d:%d:%d [%d] >f+++++++++ last_file\n", year, month, day, hour, min, sec, pid1);
                fputs(temp_line, fd2);
                sprintf(temp_line, "%d/%d/%d %d:%d:%d [%d] sent %d bytes received 0 bytes total size 0\n", year, month, day, hour, min, sec, pid1, (size1 - size2));
                fputs(temp_line, fd2);
                write = 1;
                continue;
            }
        }
        if (write == 1) {
            fputs(line, fd2);
        }
    }
    fclose(fd2);
    fclose(fd1);

    if (write == 0) {
        sprintf(cmd, "cp %s %s", filename, temp_filename);
        system(cmd);
    }
}

int
main()
{
    char filename[100] = "/root/ws/rsync-monitor/test/log.txt";
    char temp_fn[100] = "/root/ws/1";
    int ret = 0;
    int i = 0;
    char last_line[100];
    char line[100];
    transfer_file_t file_list[1000];
    memset(last_line, '\0', 100);
    memset(line, '\0', 100);

    while (1) {
        get_temp_filename(filename, temp_fn, last_line); 
        memset(file_list, 0, sizeof (transfer_file_t) * 1000);
        memset(last_line, '\0', 100);
        ret = process_logfile(temp_fn, file_list, last_line);
        //strcpy(last_line, "2014/05/06 05:49:46 [22459] sent 31 bytes received 16 bytes total size 0");

        printf("\nData transfer ...\n");
        for (i = 0; i <= ret; i ++) {
            printf("No. %d file name is %s, size is %d\n", (i+1), file_list[i].filename, file_list[i].size);
        }
        sleep(3);
    }
}
