#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>

#define MAX 1024
#define SECOND_TO_MICRO 1000000

void copy_dst(char *argv[]); /* 동기화 전에 dst 디렉토리 복사 */
void do_rsync(char *argv[]); /* src의 파일을 dst 디렉토리로 동기화 */
void remove_copy_dst(char *argv[]); /* 동기화가 완료된 경우, 복사해뒀던 dst 디렉토리 삭제 */
static void ssu_signal_handler(int signo); /* SIGINT 시그널이 발생한 경우, dst 디렉토리를 원래의 상태로 복구 */
void make_file_log(char performance_time[], char *argv[], char *filename); /* 동기화가 완료된 후에 log를 찍음 */
void make_dir_log(char performance_time[], char *argv[], char src_path[]); /* 동기화가 완료된 후에 log를 찍음 */
void insert_dir_log(char *argv[], char src_path[]); /* 동기화가 완료된 후에 log를 찍음(src가 디렉토리인 경우 내부 파일에 대해) */
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t); /* ssu_mntr의 수행시간 측정 */

char** g_argv;
int mark = 0; //시그널 받았는지 확인하기 위한 변수

int main(int argc, char* argv[])
{
    char src_absolute_path[MAX];
    char dst_absolute_path[MAX];
    
    struct stat src_statbuf;
    struct stat dst_statbuf;
    struct timeval begin_t, end_t;

    g_argv = argv;

    gettimeofday(&begin_t, NULL);

    if(access(argv[1], F_OK) != 0) {
        printf("usage: 경로에 해당하는 파일을 찾을 수 없음\n");
        exit(1);
    }

    if(realpath(argv[1], src_absolute_path) == NULL) {
        perror("realpath error\n");
        exit(1);
    }

    stat (src_absolute_path, &src_statbuf);

    if((S_ISDIR(src_statbuf.st_mode))) {
        if((access(argv[1], R_OK) != 0) && (access(argv[2], X_OK) != 0)) {
            printf("usage: 경로에 해당하는 파일의 접근 권한이 없음\n");
            exit(1);
        }
    }

    else {
        if(access(argv[1], R_OK) != 0) {
            printf("usage: 경로에 해당하는 파일의 접근 권한이 없음\n");
            exit(1);
        }
    }

    if(access(argv[2], F_OK) != 0) {
        printf("usage: 경로에 해당하는 파일을 찾을 수 없음\n");
        exit(1);
    }

    if((access(argv[2], R_OK) != 0) && (access(argv[2], W_OK) != 0) && (access(argv[2], X_OK) != 0)) {
        printf("usage: 경로에 해당하는 파일의 접근 권한이 없음\n");
        exit(1);
    }

    if(realpath(argv[2], dst_absolute_path) == NULL) {
        perror("realpath error\n");
        exit(1);
    }

    stat (dst_absolute_path, &dst_statbuf);

    if(!(S_ISDIR(dst_statbuf.st_mode))) {
        printf("usage: dst 인자가 디렉토리가 아님\n");
        exit(1);
    }

    copy_dst(argv);
    do_rsync(argv);

    if(mark == 0)
        remove_copy_dst(argv);

    gettimeofday(&end_t, NULL);
    ssu_runtime(&begin_t, &end_t);

    exit(0);
}


/**
함 수 : copy_dst(char *argv[])
기 능 : 동기화를 하기 전에 dst 디렉토리에 있던 파일 내용 복사
*/
void copy_dst(char *argv[]) {

    char ssu_rsync_path[MAX];
    char old_path[MAX];
    char new_path[MAX];
    
    struct dirent **items;
    int nitems;

    getcwd(ssu_rsync_path, sizeof(ssu_rsync_path));
    
    if(mkdir("buf_dst", 0766) == -1) {
        fprintf(stderr, "directory create error\n");
        exit(1);
    }

    nitems = scandir(argv[2], &items, NULL, alphasort); //현재 디렉토리의 모든 파일과 디렉토리 내용 가져옴
    chdir(ssu_rsync_path);

    for(int i=0; i<nitems; i++) {

        /* 현재 디렉토리, 이전 디렉토리 무시 */
        if((!strcmp(items[i]->d_name, ".")) || (!strcmp(items[i]->d_name, "..")))
            continue;

        sprintf(old_path, "%s/%s", argv[2], items[i]->d_name);
        sprintf(new_path, "buf_dst/%s", items[i]->d_name);

        if(link(old_path, new_path) == -1) {
            perror("link error\n");
            exit(1);
        }
    }
}


/**
함 수 : do_rsync(char *argv[])
기 능 : src가 파일인 경우와 디렉토리인 경우로 나누어서 dst 디렉토리에 파일들을 동기화
        (dst에 동일한 파일이 있는 경우에는 동기화를 진행하지 않음)
*/
void do_rsync(char *argv[]) {

    char *filename;
    char src_absolute_path[MAX];
    char src_path[MAX];
    char dst_path[MAX];
    char dst_absolute_path[MAX];
    char performance_time[MAX]={0,};
    char ssu_rsync_path[MAX];

    struct stat src_statbuf;
    struct stat dst_statbuf;
    time_t t;
    struct dirent **items;
    int nitems;
    
    getcwd(ssu_rsync_path, sizeof(ssu_rsync_path));

    if(strchr(argv[1], '/') != NULL) {
        filename = strrchr(argv[1], '/') + 1;
    }
    else {
        filename = malloc(strlen(argv[1])+1);
        strcpy(filename, argv[1]);
    }
    
    if(realpath(argv[1], src_absolute_path) == NULL) {
        perror("realpath error\n");
        exit(1);
    }
    
    stat (src_absolute_path, &src_statbuf);

    sprintf(dst_path, "%s/%s", argv[2], filename);

    if(access(dst_path, F_OK) == 0) {
        if(realpath(dst_path, dst_absolute_path) == NULL) {
            perror("realpath error\n");
            exit(1);
        }
        
        stat (dst_absolute_path, &dst_statbuf);
     
        /* 동일한 파일이 있는 경우 동기화를 진행하지 않음 */
        if((src_statbuf.st_size==dst_statbuf.st_size) && (src_statbuf.st_mtime==dst_statbuf.st_mtime))
            ;
        else {
            if (unlink(dst_path) < 0) {
                perror("unlink error\n");
                exit(1);
            }
            
            
            /* 동기화 대상이 디렉토리인 경우 */
            if((S_ISDIR(src_statbuf.st_mode))) {

                int cnt = 0;

                nitems = scandir(argv[1], &items, NULL, alphasort); //현재 디렉토리의 모든 파일과 디렉토리 내용 가져옴
                chdir(ssu_rsync_path);

                for(int i=0; i<nitems; i++) {

                    char absolute_path[MAX];
                    struct stat fstat; //파일 상태 저장하기 위한 구조체

                    /* 현재 디렉토리, 이전 디렉토리 무시 */
                    if((!strcmp(items[i]->d_name, ".")) || (!strcmp(items[i]->d_name, "..")))
                        continue;
                
                    sprintf(src_path, "%s/%s", argv[1], items[i]->d_name);
                    memset(dst_path, 0, MAX);
                    sprintf(dst_path, "%s/%s", argv[2], items[i]->d_name);

                    if(realpath(src_path, absolute_path) == NULL) {
                        perror("realpath error\n");
                        exit(1);
                    }

                    stat(absolute_path, &fstat);


                    if(access(dst_path, F_OK) == 0) {
                        memset(dst_absolute_path, 0, MAX);

                        if(realpath(dst_path, dst_absolute_path) == NULL) {
                            perror("realpath error\n");
                            exit(1);
                        }
        
                        stat(dst_absolute_path, &dst_statbuf);
     
                        /* 동일한 파일이 있는 경우 동기화를 진행하지 않음 */
                        if((fstat.st_size==dst_statbuf.st_size) && (fstat.st_mtime==dst_statbuf.st_mtime)) {
                            continue;
                        }
                    }

                    /* 파일이 디렉토리인 경우, 동기화 진행 X */
                    if(S_ISDIR(fstat.st_mode))
                        ;
                    else {
                        chmod(src_path, 000);
                        if (link(src_path, dst_path) == -1) {
                            perror("link error\n");
                            exit(1);
                        }

                        if (signal(SIGINT, ssu_signal_handler) == SIG_ERR) {
                            fprintf(stderr, "cannot handle SIGINT\n");
                            exit(EXIT_FAILURE);
                        }
                        chmod(src_path, fstat.st_mode);

                        cnt++;

                        t = time(NULL);
                        sprintf(performance_time, "%s", ctime(&t));
                        performance_time[strlen(performance_time) - 1] = '\0'; //개행문자 제거

                        if (mark == 0) {
                            if(cnt==1)
                                make_dir_log(performance_time, argv, src_path);
                            else
                                insert_dir_log(argv, src_path);
                            }
                        }

                    memset(&fstat, 0, sizeof(fstat));
                }
            }


            /* 동기화 대상이 파일인 경우 */
            else {
                chmod(argv[1], 000);
                if (link(argv[1], dst_path) == -1) {
                    perror("link error\n");
                    exit(1);
                }

                if (signal(SIGINT, ssu_signal_handler) == SIG_ERR) {
                    fprintf(stderr, "cannot handle SIGINT\n");
                    exit(EXIT_FAILURE);
                }
                chmod(argv[1], src_statbuf.st_mode);

                t = time(NULL);
                sprintf(performance_time, "%s", ctime(&t));
                performance_time[strlen(performance_time) - 1] = '\0'; //개행문자 제거
                
                if (mark ==0)
                    make_file_log(performance_time, argv, filename);
            } 
        }
    }

    else {
        /* 동기화 대상이 디렉토리인 경우 */
        if((S_ISDIR(src_statbuf.st_mode))) {

            int cnt = 0;

            nitems = scandir(argv[1], &items, NULL, alphasort); //현재 디렉토리의 모든 파일과 디렉토리 내용 가져옴
            chdir(ssu_rsync_path);

            for(int i=0; i<nitems; i++) {

                char absolute_path[MAX];
                struct stat fstat; //파일 상태 저장하기 위한 구조체

                /* 현재 디렉토리, 이전 디렉토리 무시 */
                if((!strcmp(items[i]->d_name, ".")) || (!strcmp(items[i]->d_name, "..")))
                    continue;
                
                sprintf(src_path, "%s/%s", argv[1], items[i]->d_name);
                memset(dst_path, 0, MAX);
                sprintf(dst_path, "%s/%s", argv[2], items[i]->d_name);

                if(realpath(src_path, absolute_path) == NULL) {
                    perror("realpath error\n");
                    exit(1);
                }

                stat(absolute_path, &fstat);

                if(access(dst_path, F_OK) == 0) {
                    memset(dst_absolute_path, 0, MAX);

                    if(realpath(dst_path, dst_absolute_path) == NULL) {
                        perror("realpath error\n");
                        exit(1);
                    }
        
                    stat(dst_absolute_path, &dst_statbuf);
     
                    /* 동일한 파일이 있는 경우 동기화를 진행하지 않음 */
                    if((fstat.st_size==dst_statbuf.st_size) && (fstat.st_mtime==dst_statbuf.st_mtime)) {
                        continue;
                    }
                }

                /* 파일이 디렉토리인 경우, 동기화 진행 X */
                if(S_ISDIR(fstat.st_mode))
                    ;
                else {
                    chmod(src_path, 000);
                    if (link(src_path, dst_path) == -1) {
                        perror("link error\n");
                        exit(1);
                    }

                    if (signal(SIGINT, ssu_signal_handler) == SIG_ERR) {
                        fprintf(stderr, "cannot handle SIGINT\n");
                        exit(EXIT_FAILURE);
                    }
                    chmod(src_path, fstat.st_mode);

                    cnt++;

                    t = time(NULL);
                    sprintf(performance_time, "%s", ctime(&t));
                    performance_time[strlen(performance_time) - 1] = '\0'; //개행문자 제거

                    if (mark == 0) {
                        if(cnt==1)
                            make_dir_log(performance_time, argv, src_path);
                        else
                            insert_dir_log(argv, src_path);
                    }
                }
                memset(&fstat, 0, sizeof(fstat));
            }

        }

        /* 동기화 대상이 파일인 경우 */
        else {
            chmod(argv[1], 000);
            if (link(argv[1], dst_path) == -1) {
                perror("link error\n");
                exit(1);
            }

            if (signal(SIGINT, ssu_signal_handler) == SIG_ERR) {
                fprintf(stderr, "cannot handle SIGINT\n");
                exit(EXIT_FAILURE);
            }
            chmod(argv[1], src_statbuf.st_mode);

            t = time(NULL);
            sprintf(performance_time, "%s", ctime(&t));
            performance_time[strlen(performance_time) - 1] = '\0'; //개행문자 제거
                
            if (mark == 0)
                make_file_log(performance_time, argv, filename);
        } 
    }
}


/**
함 수 : remove_copy_dst(char *argv[])
기 능 : 동기화 도중에 SIGINT 시그널을 받지 않아 정상적으로 동기화를 진행했을 경우, copy 해뒀던 dst 디렉토리를 삭제
*/
void remove_copy_dst(char *argv[]) {

    char ssu_rsync_path[MAX];
    char remove_path[MAX];
    
    struct dirent **items;
    int nitems;

    getcwd(ssu_rsync_path, sizeof(ssu_rsync_path));

    nitems = scandir("buf_dst", &items, NULL, alphasort); //현재 디렉토리의 모든 파일과 디렉토리 내용 가져옴
    chdir(ssu_rsync_path);

    for(int i=0; i<nitems; i++) {

        /* 현재 디렉토리, 이전 디렉토리 무시 */
        if((!strcmp(items[i]->d_name, ".")) || (!strcmp(items[i]->d_name, "..")))
            continue;

        sprintf(remove_path, "buf_dst/%s", items[i]->d_name);

        if(unlink(remove_path) < 0) { //buf_dst 디렉토리 안에 파일 삭제
            perror("unlink error\n");
            exit(1);
        }
    }

    if(remove("buf_dst") < 0) { //buf_dst 디렉토리 삭제
        perror("rename error\n");
        exit(1);
    }
}


/**
함 수 : ssu_signal_handler(int signo)
기 능 : 동기화 도중에 SIGINT 시그널을 받았을 경우에 dst 디렉토리의 내용을 전부 삭제하고, copy해두었던 dst 디렉토리의 파일을 dst 디렉토리로 옮김
*/
static void ssu_signal_handler(int signo) {

    char ssu_rsync_path[MAX];
    char remove_path[MAX];
    char old_path[MAX];
    char new_path[MAX];
    
    struct dirent **items1;
    int nitems1;
    struct dirent **items2;
    int nitems2;

    mark = 1;

    getcwd(ssu_rsync_path, sizeof(ssu_rsync_path));
    
    nitems1 = scandir(g_argv[2], &items1, NULL, alphasort); //현재 디렉토리의 모든 파일과 디렉토리 내용 가져옴
    chdir(ssu_rsync_path);

    for(int i=0; i<nitems1; i++) {
        
        /* 현재 디렉토리, 이전 디렉토리 무시 */
        if((!strcmp(items1[i]->d_name, ".")) || (!strcmp(items1[i]->d_name, "..")))
            continue;
        
        sprintf(remove_path, "%s/%s", g_argv[2], items1[i]->d_name);
     
        if(unlink(remove_path) < 0) { //dst 디렉토리 안에 파일 삭제
            perror("unlink error\n");
            exit(1);
        }
    }
   
    nitems2 = scandir("buf_dst", &items2, NULL, alphasort); //현재 디렉토리의 모든 파일과 디렉토리 내용 가져옴
    chdir(ssu_rsync_path);

    for(int i=0; i<nitems2; i++) {

        /* 현재 디렉토리, 이전 디렉토리 무시 */
        if((!strcmp(items2[i]->d_name, ".")) || (!strcmp(items2[i]->d_name, "..")))
            continue;

        sprintf(old_path, "buf_dst/%s", items2[i]->d_name);
        sprintf(new_path, "%s/%s", g_argv[2], items2[i]->d_name);
      
        if(rename(old_path, new_path) == -1) { //파일 원래 경로로 복구
            perror("rename error\n");
            exit(1);
        }
     
    }
   
    if(remove("buf_dst") < 0) { //buf_dst 디렉토리 삭제
        perror("remove error\n");
        exit(1);
    }
}


/**
함 수 : make_file_log(char performance_time[], char *argv[], char *filename)
기 능 : 동기화가 완료된 경우 동기화된 내용을 log로 찍음
*/
void make_file_log(char performance_time[], char *argv[], char *filename) {

    char log_title[MAX]={0,};
    char src_absolute_path[MAX];
    char log_contents[MAX]={0,};
    
    FILE *fp;
    struct stat statbuf;

    if((fp = fopen("ssu_rsync_log", "a+")) == NULL) { //ssu_rsync_log open (파일이 없으면 생성)
        perror("fopen error\n");
        exit(1);
    }

    sprintf(log_title, "[%s] ssu_rsync %s %s", performance_time, argv[1], argv[2]);
    fprintf(fp, "%s\n", log_title);


    if(realpath(argv[1], src_absolute_path) == NULL) {
        perror("realpath error\n");
        exit(1);
    }

    stat(src_absolute_path, &statbuf);

    sprintf(log_contents, "         %s %ldbytes", filename, statbuf.st_size);
    fprintf(fp, "%s\n", log_contents);

    fclose(fp);
}


/**
함 수 : make_dir_log(char performance_time[], char *argv[], char src_path[])
기 능 : 동기화가 완료된 경우 동기화된 내용을 log로 찍음
*/
void make_dir_log(char performance_time[], char *argv[], char src_path[]) {

    char log_title[MAX]={0,};
    char src_absolute_path[MAX];
    char *filename;
    char log_contents[MAX]={0,};
    
    FILE *fp;
    struct stat statbuf;

    if((fp = fopen("ssu_rsync_log", "a+")) == NULL) { //ssu_rsync_log open (파일이 없으면 생성)
        perror("fopen error\n");
        exit(1);
    }

    sprintf(log_title, "[%s] ssu_rsync %s %s", performance_time, argv[1], argv[2]);
    fprintf(fp, "%s\n", log_title);

    if(realpath(src_path, src_absolute_path) == NULL) {
        perror("realpath error\n");
        exit(1);
    }

    filename = src_path;
    while(strchr(filename, '/') != NULL) {
        filename++;
    }

    stat(src_absolute_path, &statbuf);

    sprintf(log_contents, "         %s %ldbytes", filename, statbuf.st_size);
    fprintf(fp, "%s\n", log_contents);

    fclose(fp);
}


/**
함 수 : insert_dir_log(char *argv[], char src_path[])
기 능 : 동기화 대상이 디렉토리였을 경우에 동기화한 디렉토리 내부의 파일을 전부 log로 찍음
*/
void insert_dir_log(char *argv[], char src_path[]) {

    char src_absolute_path[MAX];
    char *filename;
    char log_contents[MAX]={0,};
    
    FILE *fp;
    struct stat statbuf;

    if((fp = fopen("ssu_rsync_log", "a")) == NULL) { //ssu_rsync_log open
        perror("fopen error\n");
        exit(1);
    }

    if(realpath(src_path, src_absolute_path) == NULL) {
        perror("realpath error\n");
        exit(1);
    }

    filename = src_path;
    while(strchr(filename, '/') != NULL) {
        filename++;
    }

    stat(src_absolute_path, &statbuf);

    sprintf(log_contents, "         %s %ldbytes", filename, statbuf.st_size);
    fprintf(fp, "%s\n", log_contents);

    fclose(fp);
}


/**
함 수 : ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
기 능 : ssu_crontab의 수행 시간을 측정
*/
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t) {
    
    end_t->tv_sec -= begin_t->tv_sec;

    if(end_t->tv_usec < begin_t->tv_usec) {
        end_t->tv_sec--;
        end_t->tv_usec += SECOND_TO_MICRO;
    }

    end_t->tv_usec -= begin_t->tv_usec;
    printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);
}