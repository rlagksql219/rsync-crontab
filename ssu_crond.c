#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

#define MAX 1024

int ssu_daemon_init(void); /* 현재의 프로세스를 데몬 프로세스로 만듬 */
void run_log(char performance_time[], char execution_cycle[], char instructions[]); /* ssu_crontab_file에 등록된 명령어가 실행됐을 경우에 log 남김 */
int check_minute(char minute[]); /* 현재의 시간(분)과 비교하여 명령어 실행여부 결정 */
int check_hour(char hour[]); /* 현재의 시간(시)과 비교하여 명령어 실행여부 결정 */
int check_day(char day[]); /* 현재의 날짜(일)와 비교하여 명령어 실행여부 결정 */
int check_month(char month[]); /* 현재의 날짜(월)와 비교하여 명령어 실행여부 결정 */
int check_dayofweek(char dayofweek[]); /* 현재의 날짜(요일)와 비교하여 명령어 실행여부 결정 */

int main()
{
    char file_contents[MAX]={0,};
    char ssu_crontab_file_absolute_path[MAX]={0,};
    char performance_time[MAX]={0,};

    FILE *fp;
    time_t t;
    pid_t pid;
  
    if (ssu_daemon_init() < 0) {
        fprintf(stderr, "ssu_daemon_init failed\n");
        exit(1);
    }

    if ((pid = fork()) < 0) {
        fprintf(stderr, "fork error\n");
        exit(1);
    }
   
    while(1) {
        if((fp = fopen("ssu_crontab_file", "r")) == NULL) { //ssu_crontab_file open
            perror("fopen error\n");
            exit(1);
        }
        
        while(fgets(file_contents, MAX, fp) != NULL) {
            char cpy_file_contents[MAX] = {0,};
	        char *ptr;
            char minute[MAX] = {0,};
            char hour[MAX] = {0,};
            char day[MAX] = {0,};
            char month[MAX] = {0,};
            char dayofweek[MAX] = {0,};
            char instructions[MAX] = {0,};
            char execution_cycle[MAX]={0,};

            strcpy(cpy_file_contents, file_contents);
            strtok(cpy_file_contents, " ");
            ptr = strtok(NULL, " ");
            strcpy(minute, ptr);
            ptr = strtok(NULL, " ");
            strcpy(hour, ptr);
            ptr = strtok(NULL, " ");
            strcpy(day, ptr);
            ptr = strtok(NULL, " ");
            strcpy(month, ptr);
            ptr = strtok(NULL, " ");
            strcpy(dayofweek, ptr);

            ptr = strtok(NULL, "\0");
            strcpy(instructions, ptr);
            
            if (check_minute(minute) == 1 && check_hour(hour) == 1 && check_day(day) == 1 && check_month(month) == 1 && check_dayofweek(dayofweek) == 1) {

                t = time(NULL);

                if (pid == 0) { //자식
                    system(instructions);
                }

                else { //부모
                    sprintf(performance_time, "%s", ctime(&t));
                    performance_time[strlen(performance_time) - 1] = '\0'; //개행문자 제거

                    sprintf(execution_cycle, "%s %s %s %s %s", minute, hour, day, month, dayofweek);

                    run_log(performance_time, execution_cycle, instructions);
                }
            }

            memset(file_contents, 0, MAX);
        }

        fclose(fp);
        sleep(60);
    }

    exit(0);
}


/**
함 수 : ssu_daemon_init(void)
기 능 : 해당 프로세스를 데몬 프로세스로 만듬
*/
int ssu_daemon_init(void) {
    pid_t pid;
    int fd, maxfd;

    if ((pid = fork()) < 0) {
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    else if (pid != 0)
        exit(0);

    pid = getpid();
    //printf("process %d running as daemon\n", pid);
    setsid();
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    maxfd = getdtablesize();

    for (fd=0; fd<maxfd; fd++)
       close(fd);

    umask(0);
    fd = open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    return 0;
}


/**
함 수 : run_log(char performance_time[], char execution_cycle[], char instructions[])
기 능 : ssu_crontab_file에 등록된 명령어가 실행됐을 경우에 log 남김
*/
void run_log(char performance_time[], char execution_cycle[], char instructions[]) {
    char log_contents[MAX]={0,};
    
    FILE *fp;

    if((fp = fopen("ssu_crontab_log", "a+")) == NULL) { //ssu_crontab_log open (파일이 없으면 생성)
        perror("fopen error\n");
        exit(1);
    }

    sprintf(log_contents, "[%s] run %s %s", performance_time, execution_cycle, instructions);
    fprintf(fp, "%s\n", log_contents);

    fclose(fp);
}


/**
함 수 : check_minute(char minute[])
기 능 : 현재의 시간(분)과 비교하여 명령어 실행여부 결정
*/
int check_minute(char minute[]) {
    int cur_minute;
    
    time_t t;
    struct tm tm;

    t = time(NULL);
    tm = *localtime(&t);
    cur_minute = tm.tm_min;
    
    /* 주기에 '-', ',', '/' 포함 */
    if ((strchr(minute, '-') != NULL) && (strchr(minute, ',') != NULL) && (strchr(minute, '/') != NULL)) {
        char cpy_minute[MAX] = {0,};
	    char *ptr1;
        char *ptr2;
        char *ptr3;
        char *ptr4;
        char *ptr5;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};
        char before_slash1[MAX] = {0,};
        char after_slash1[MAX] = {0,}; //execution 3
        char before_slash2[MAX] = {0,};
        char after_slash2[MAX] = {0,}; //execution 6
        char before_minus1[MAX] = {0,}; //execution 1
        char after_minus1[MAX] = {0,}; //execution 2
        char before_minus2[MAX] = {0,}; //execution 4
        char after_minus2[MAX] = {0,}; //execution 5
        char cpy_before_comma[MAX] = {0,};
        char cpy_after_comma[MAX] = {0,};
        char cpy_before_slash1[MAX] = {0,};
        char cpy_before_slash2[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_minute, minute);
        ptr1 = strtok(cpy_minute, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_before_comma, before_comma);
        ptr2 = strtok(cpy_before_comma, "/");
        strcpy(before_slash1, ptr2);
        ptr2 = strtok(NULL, "/");
        strcpy(after_slash1, ptr2);

        strcpy(cpy_after_comma, after_comma);
        ptr3 = strtok(cpy_after_comma, "/");
        strcpy(before_slash2, ptr3);
        ptr3 = strtok(NULL, "/");
        strcpy(after_slash2, ptr3);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash1, before_slash1);
        ptr4 = strtok(cpy_before_slash1, "-");
        strcpy(before_minus1, ptr4);
        ptr4 = strtok(NULL, "-");
        strcpy(after_minus1, ptr4);

        strcpy(cpy_before_slash2, before_slash2);
        ptr5 = strtok(cpy_before_slash2, "-");
        strcpy(before_minus2, ptr5);
        ptr5 = strtok(NULL, "-");
        strcpy(after_minus2, ptr5);

        int num1 = atoi(before_minus1) % atoi(after_slash1);
        int num2 = atoi(before_minus2) % atoi(after_slash2);
        if (((atoi(before_minus1) <= cur_minute) && (atoi(after_minus1) >= cur_minute) && (cur_minute % atoi(after_slash1) == num1)) || ((atoi(before_minus2) <= cur_minute) && (atoi(after_minus2) >= cur_minute) && (cur_minute % atoi(after_slash2) == num2)))
            return 1;

        // printf("1 %s\n", before_minus1);
        // printf("2 %s\n", after_minus1);
        // printf("3 %s\n", after_slash1);
        // printf("4 %s\n", before_minus2);
        // printf("5 %s\n", after_minus2);
        // printf("6 %s\n", after_slash2);
    }

    /* 주기에 '-', ',' 포함 */
    else if ((strchr(minute, '-') != NULL) && (strchr(minute, ',') != NULL) && (strchr(minute, '/') == NULL)) {
        char cpy_minute[MAX] = {0,};
        char *ptr1;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_minute, minute);
        ptr1 = strtok(cpy_minute, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* ex. 1-10, 30-40 */
        if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char *ptr3;
            char before_minus1[MAX] = {0,}; //execution 1
            char after_minus1[MAX] = {0,}; //execution 2
            char before_minus2[MAX] = {0,}; //execution 3
            char after_minus2[MAX] = {0,}; //execution 4

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus1, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus1, ptr2);

            strcpy(cpy_after_comma, after_comma);
            ptr3 = strtok(cpy_after_comma, "-");
            strcpy(before_minus2, ptr3);
            ptr3 = strtok(NULL, "-");
            strcpy(after_minus2, ptr3);

            if (((atoi(before_minus1) <= cur_minute) && (atoi(after_minus1) >= cur_minute)) || ((atoi(before_minus2) <= cur_minute) && (atoi(after_minus2) >= cur_minute)))
            return 1;

            // printf("1 %s\n", before_minus1);
            // printf("2 %s\n", after_minus1);
            // printf("3 %s\n", before_minus2);
            // printf("4 %s\n", after_minus2);
        }
        /* ex. 1-10, 30 */
        else if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') == NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 1
            char after_minus[MAX] = {0,}; //execution 2
            //after_comma => execution 3

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_minute) && (atoi(after_minus) >= cur_minute)) || ((atoi(after_comma) == cur_minute) || (strchr(after_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_minus);
            // printf("2 %s\n", after_minus);
            // printf("3 %s\n", after_comma);
        }
        /* ex. 1, 30-40 */
        else if((strchr(before_comma, '-') == NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 2
            char after_minus[MAX] = {0,}; //execution 3
            //before_comma => execution 1

            strcpy(cpy_after_comma, after_comma);
            ptr2 = strtok(cpy_after_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_minute) && (atoi(after_minus) >= cur_minute)) || ((atoi(before_comma) == cur_minute) || (strchr(before_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_comma);
            // printf("2 %s\n", before_minus);
            // printf("3 %s\n", after_minus);
        }     
    }
    /* 주기에 '-', '/' 포함 */
    else if ((strchr(minute, '-') != NULL) && (strchr(minute, ',') == NULL) && (strchr(minute, '/') != NULL)) {
        char cpy_minute[MAX] = {0,};
        char *ptr1;
        char *ptr2;
        char before_slash[MAX] = {0,};
        char after_slash[MAX] = {0,}; //execution 3
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2
        char cpy_before_slash[MAX] = {0,};

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_minute, minute);
        ptr1 = strtok(cpy_minute, "/");
        strcpy(before_slash, ptr1);
        ptr1 = strtok(NULL, "/");
        strcpy(after_slash, ptr1);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash, before_slash);
        ptr2 = strtok(cpy_before_slash, "-");
        strcpy(before_minus, ptr2);
        ptr2 = strtok(NULL, "-");
        strcpy(after_minus, ptr2);

        int num = atoi(before_minus) % atoi(after_slash);
        if (((atoi(before_minus) <= cur_minute) && (atoi(after_minus) >= cur_minute) && (cur_minute % atoi(after_slash) == num)))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
        // printf("3 %s\n", after_slash);
    }
    // /* 주기에 ',', '/' 포함 */
    // else if ((strchr(minute, '-') == NULL) && (strchr(minute, ',') != NULL) && (strchr(minute, '/') != NULL)) {
    //     //에러처리
    // }

    /* 주기에 '-' 포함 */
    else if ((strchr(minute, '-') != NULL) && (strchr(minute, ',') == NULL) && (strchr(minute, '/') == NULL)) {
        char cpy_minute[MAX] = {0,};
        char *ptr;
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2

        strcpy(cpy_minute, minute);
        ptr = strtok(cpy_minute, "-");
        strcpy(before_minus, ptr);
        ptr = strtok(NULL, "-");
        strcpy(after_minus, ptr);

        if ((atoi(before_minus) <= cur_minute) && (atoi(after_minus) >= cur_minute))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
    }
    /* 주기에 ',' 포함 */
    else if ((strchr(minute, '-') == NULL) && (strchr(minute, ',') != NULL) && (strchr(minute, '/') == NULL)) {
        char cpy_minute[MAX] = {0,};
        char *ptr;
        char before_comma[MAX] = {0,}; //execution 1
        char after_comma[MAX] = {0,}; //execution 2

        strcpy(cpy_minute, minute);
        ptr = strtok(cpy_minute, ",");
        strcpy(before_comma, ptr);
        ptr = strtok(NULL, ",");
        strcpy(after_comma, ptr);

        if ((atoi(before_comma) == cur_minute) || (atoi(after_comma) == cur_minute) || (strchr(before_comma, '*') != NULL) || (strchr(after_comma, '*') != NULL))
            return 1;

        // printf("1 %s\n", before_comma);
        // printf("2 %s\n", after_comma);
    }
    /* 주기에 '/' 포함 */
    else if ((strchr(minute, '-') == NULL) && (strchr(minute, ',') == NULL) && (strchr(minute, '/') != NULL)) {
        char cpy_minute[MAX] = {0,};
        char *ptr;
        char before_slash[MAX] = {0,}; //execution 1
        char after_slash[MAX] = {0,}; //execution 2

        strcpy(cpy_minute, minute);
        ptr = strtok(cpy_minute, "/");
        strcpy(before_slash, ptr);
        ptr = strtok(NULL, "/");
        strcpy(after_slash, ptr);

        if ((strchr(before_slash, '*') != NULL) && (cur_minute % atoi(after_slash) == 0))
            return 1;

        // printf("1 %s\n", before_slash);
        // printf("2 %s\n", after_slash);
    }

    /* 주기에 '*' 혹은 숫자만 포함 */
    else {
        if((strchr(minute, '*') != NULL) || (atoi(minute) == cur_minute))
            return 1;
    }

    return 0;
}


/**
함 수 : check_hour(char hour[])
기 능 : 현재의 시간(시)과 비교하여 명령어 실행여부 결정
*/
int check_hour(char hour[]) {
    int cur_hour;
    
    time_t t;
    struct tm tm;

    t = time(NULL);
    tm = *localtime(&t);
    cur_hour = tm.tm_hour;
    
    /* 주기에 '-', ',', '/' 포함 */
    if ((strchr(hour, '-') != NULL) && (strchr(hour, ',') != NULL) && (strchr(hour, '/') != NULL)) {
        char cpy_hour[MAX] = {0,};
	    char *ptr1;
        char *ptr2;
        char *ptr3;
        char *ptr4;
        char *ptr5;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};
        char before_slash1[MAX] = {0,};
        char after_slash1[MAX] = {0,}; //execution 3
        char before_slash2[MAX] = {0,};
        char after_slash2[MAX] = {0,}; //execution 6
        char before_minus1[MAX] = {0,}; //execution 1
        char after_minus1[MAX] = {0,}; //execution 2
        char before_minus2[MAX] = {0,}; //execution 4
        char after_minus2[MAX] = {0,}; //execution 5
        char cpy_before_comma[MAX] = {0,};
        char cpy_after_comma[MAX] = {0,};
        char cpy_before_slash1[MAX] = {0,};
        char cpy_before_slash2[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_hour, hour);
        ptr1 = strtok(cpy_hour, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_before_comma, before_comma);
        ptr2 = strtok(cpy_before_comma, "/");
        strcpy(before_slash1, ptr2);
        ptr2 = strtok(NULL, "/");
        strcpy(after_slash1, ptr2);

        strcpy(cpy_after_comma, after_comma);
        ptr3 = strtok(cpy_after_comma, "/");
        strcpy(before_slash2, ptr3);
        ptr3 = strtok(NULL, "/");
        strcpy(after_slash2, ptr3);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash1, before_slash1);
        ptr4 = strtok(cpy_before_slash1, "-");
        strcpy(before_minus1, ptr4);
        ptr4 = strtok(NULL, "-");
        strcpy(after_minus1, ptr4);

        strcpy(cpy_before_slash2, before_slash2);
        ptr5 = strtok(cpy_before_slash2, "-");
        strcpy(before_minus2, ptr5);
        ptr5 = strtok(NULL, "-");
        strcpy(after_minus2, ptr5);

        int num1 = atoi(before_minus1) % atoi(after_slash1);
        int num2 = atoi(before_minus2) % atoi(after_slash2);
        if (((atoi(before_minus1) <= cur_hour) && (atoi(after_minus1) >= cur_hour) && (cur_hour % atoi(after_slash1) == num1)) || ((atoi(before_minus2) <= cur_hour) && (atoi(after_minus2) >= cur_hour) && (cur_hour % atoi(after_slash2) == num2)))
            return 1;

        // printf("1 %s\n", before_minus1);
        // printf("2 %s\n", after_minus1);
        // printf("3 %s\n", after_slash1);
        // printf("4 %s\n", before_minus2);
        // printf("5 %s\n", after_minus2);
        // printf("6 %s\n", after_slash2);
    }

    /* 주기에 '-', ',' 포함 */
    else if ((strchr(hour, '-') != NULL) && (strchr(hour, ',') != NULL) && (strchr(hour, '/') == NULL)) {
        char cpy_hour[MAX] = {0,};
        char *ptr1;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_hour, hour);
        ptr1 = strtok(cpy_hour, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* ex. 1-5, 15-20 */
        if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char *ptr3;
            char before_minus1[MAX] = {0,}; //execution 1
            char after_minus1[MAX] = {0,}; //execution 2
            char before_minus2[MAX] = {0,}; //execution 3
            char after_minus2[MAX] = {0,}; //execution 4

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus1, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus1, ptr2);

            strcpy(cpy_after_comma, after_comma);
            ptr3 = strtok(cpy_after_comma, "-");
            strcpy(before_minus2, ptr3);
            ptr3 = strtok(NULL, "-");
            strcpy(after_minus2, ptr3);

            if (((atoi(before_minus1) <= cur_hour) && (atoi(after_minus1) >= cur_hour)) || ((atoi(before_minus2) <= cur_hour) && (atoi(after_minus2) >= cur_hour)))
            return 1;

            // printf("1 %s\n", before_minus1);
            // printf("2 %s\n", after_minus1);
            // printf("3 %s\n", before_minus2);
            // printf("4 %s\n", after_minus2);
        }
        /* ex. 1-5, 15 */
        else if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') == NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 1
            char after_minus[MAX] = {0,}; //execution 2
            //after_comma => execution 3

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_hour) && (atoi(after_minus) >= cur_hour)) || ((atoi(after_comma) == cur_hour) || (strchr(after_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_minus);
            // printf("2 %s\n", after_minus);
            // printf("3 %s\n", after_comma);
        }
        /* ex. 1, 15-20 */
        else if((strchr(before_comma, '-') == NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 2
            char after_minus[MAX] = {0,}; //execution 3
            //before_comma => execution 1

            strcpy(cpy_after_comma, after_comma);
            ptr2 = strtok(cpy_after_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_hour) && (atoi(after_minus) >= cur_hour)) || ((atoi(before_comma) == cur_hour) || (strchr(before_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_comma);
            // printf("2 %s\n", before_minus);
            // printf("3 %s\n", after_minus);
        }     
    }
    /* 주기에 '-', '/' 포함 */
    else if ((strchr(hour, '-') != NULL) && (strchr(hour, ',') == NULL) && (strchr(hour, '/') != NULL)) {
        char cpy_hour[MAX] = {0,};
        char *ptr1;
        char *ptr2;
        char before_slash[MAX] = {0,};
        char after_slash[MAX] = {0,}; //execution 3
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2
        char cpy_before_slash[MAX] = {0,};

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_hour, hour);
        ptr1 = strtok(cpy_hour, "/");
        strcpy(before_slash, ptr1);
        ptr1 = strtok(NULL, "/");
        strcpy(after_slash, ptr1);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash, before_slash);
        ptr2 = strtok(cpy_before_slash, "-");
        strcpy(before_minus, ptr2);
        ptr2 = strtok(NULL, "-");
        strcpy(after_minus, ptr2);

        int num = atoi(before_minus) % atoi(after_slash);
        if (((atoi(before_minus) <= cur_hour) && (atoi(after_minus) >= cur_hour) && (cur_hour % atoi(after_slash) == num)))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
        // printf("3 %s\n", after_slash);
    }
    // /* 주기에 ',', '/' 포함 */
    // else if ((strchr(hour, '-') == NULL) && (strchr(hour, ',') != NULL) && (strchr(hour, '/') != NULL)) {
    //     //에러처리
    // }

    /* 주기에 '-' 포함 */
    else if ((strchr(hour, '-') != NULL) && (strchr(hour, ',') == NULL) && (strchr(hour, '/') == NULL)) {
        char cpy_hour[MAX] = {0,};
        char *ptr;
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2

        strcpy(cpy_hour, hour);
        ptr = strtok(cpy_hour, "-");
        strcpy(before_minus, ptr);
        ptr = strtok(NULL, "-");
        strcpy(after_minus, ptr);

        if ((atoi(before_minus) <= cur_hour) && (atoi(after_minus) >= cur_hour))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
    }
    /* 주기에 ',' 포함 */
    else if ((strchr(hour, '-') == NULL) && (strchr(hour, ',') != NULL) && (strchr(hour, '/') == NULL)) {
        char cpy_hour[MAX] = {0,};
        char *ptr;
        char before_comma[MAX] = {0,}; //execution 1
        char after_comma[MAX] = {0,}; //execution 2

        strcpy(cpy_hour, hour);
        ptr = strtok(cpy_hour, ",");
        strcpy(before_comma, ptr);
        ptr = strtok(NULL, ",");
        strcpy(after_comma, ptr);

        if ((atoi(before_comma) == cur_hour) || (atoi(after_comma) == cur_hour) || (strchr(before_comma, '*') != NULL) || (strchr(after_comma, '*') != NULL))
            return 1;

        // printf("1 %s\n", before_comma);
        // printf("2 %s\n", after_comma);
    }
    /* 주기에 '/' 포함 */
    else if ((strchr(hour, '-') == NULL) && (strchr(hour, ',') == NULL) && (strchr(hour, '/') != NULL)) {
        char cpy_hour[MAX] = {0,};
        char *ptr;
        char before_slash[MAX] = {0,}; //execution 1
        char after_slash[MAX] = {0,}; //execution 2

        strcpy(cpy_hour, hour);
        ptr = strtok(cpy_hour, "/");
        strcpy(before_slash, ptr);
        ptr = strtok(NULL, "/");
        strcpy(after_slash, ptr);

        if ((strchr(before_slash, '*') != NULL) && (cur_hour % atoi(after_slash) == 0))
            return 1;

        // printf("1 %s\n", before_slash);
        // printf("2 %s\n", after_slash);
    }

    /* 주기에 '*' 혹은 숫자만 포함 */
    else {
        if((strchr(hour, '*') != NULL) || (atoi(hour) == cur_hour))
            return 1;
    }

    return 0;
}


/**
함 수 : check_day(char day[])
기 능 : 현재의 날짜(일)와 비교하여 명령어 실행여부 결정
*/
int check_day(char day[]) {
    int cur_day;
    
    time_t t;
    struct tm tm;

    t = time(NULL);
    tm = *localtime(&t);
    cur_day = tm.tm_mday;
    
    /* 주기에 '-', ',', '/' 포함 */
    if ((strchr(day, '-') != NULL) && (strchr(day, ',') != NULL) && (strchr(day, '/') != NULL)) {
        char cpy_day[MAX] = {0,};
	    char *ptr1;
        char *ptr2;
        char *ptr3;
        char *ptr4;
        char *ptr5;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};
        char before_slash1[MAX] = {0,};
        char after_slash1[MAX] = {0,}; //execution 3
        char before_slash2[MAX] = {0,};
        char after_slash2[MAX] = {0,}; //execution 6
        char before_minus1[MAX] = {0,}; //execution 1
        char after_minus1[MAX] = {0,}; //execution 2
        char before_minus2[MAX] = {0,}; //execution 4
        char after_minus2[MAX] = {0,}; //execution 5
        char cpy_before_comma[MAX] = {0,};
        char cpy_after_comma[MAX] = {0,};
        char cpy_before_slash1[MAX] = {0,};
        char cpy_before_slash2[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_day, day);
        ptr1 = strtok(cpy_day, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_before_comma, before_comma);
        ptr2 = strtok(cpy_before_comma, "/");
        strcpy(before_slash1, ptr2);
        ptr2 = strtok(NULL, "/");
        strcpy(after_slash1, ptr2);

        strcpy(cpy_after_comma, after_comma);
        ptr3 = strtok(cpy_after_comma, "/");
        strcpy(before_slash2, ptr3);
        ptr3 = strtok(NULL, "/");
        strcpy(after_slash2, ptr3);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash1, before_slash1);
        ptr4 = strtok(cpy_before_slash1, "-");
        strcpy(before_minus1, ptr4);
        ptr4 = strtok(NULL, "-");
        strcpy(after_minus1, ptr4);

        strcpy(cpy_before_slash2, before_slash2);
        ptr5 = strtok(cpy_before_slash2, "-");
        strcpy(before_minus2, ptr5);
        ptr5 = strtok(NULL, "-");
        strcpy(after_minus2, ptr5);

        int num1 = atoi(before_minus1) % atoi(after_slash1);
        int num2 = atoi(before_minus2) % atoi(after_slash2);
        if (((atoi(before_minus1) <= cur_day) && (atoi(after_minus1) >= cur_day) && (cur_day % atoi(after_slash1) == num1)) || ((atoi(before_minus2) <= cur_day) && (atoi(after_minus2) >= cur_day) && (cur_day % atoi(after_slash2) == num2)))
            return 1;

        // printf("1 %s\n", before_minus1);
        // printf("2 %s\n", after_minus1);
        // printf("3 %s\n", after_slash1);
        // printf("4 %s\n", before_minus2);
        // printf("5 %s\n", after_minus2);
        // printf("6 %s\n", after_slash2);
    }

    /* 주기에 '-', ',' 포함 */
    else if ((strchr(day, '-') != NULL) && (strchr(day, ',') != NULL) && (strchr(day, '/') == NULL)) {
        char cpy_day[MAX] = {0,};
        char *ptr1;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_day, day);
        ptr1 = strtok(cpy_day, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* ex. 1-10, 15-30 */
        if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char *ptr3;
            char before_minus1[MAX] = {0,}; //execution 1
            char after_minus1[MAX] = {0,}; //execution 2
            char before_minus2[MAX] = {0,}; //execution 3
            char after_minus2[MAX] = {0,}; //execution 4

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus1, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus1, ptr2);

            strcpy(cpy_after_comma, after_comma);
            ptr3 = strtok(cpy_after_comma, "-");
            strcpy(before_minus2, ptr3);
            ptr3 = strtok(NULL, "-");
            strcpy(after_minus2, ptr3);

            if (((atoi(before_minus1) <= cur_day) && (atoi(after_minus1) >= cur_day)) || ((atoi(before_minus2) <= cur_day) && (atoi(after_minus2) >= cur_day)))
            return 1;

            // printf("1 %s\n", before_minus1);
            // printf("2 %s\n", after_minus1);
            // printf("3 %s\n", before_minus2);
            // printf("4 %s\n", after_minus2);
        }
        /* ex. 1-10, 15 */
        else if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') == NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 1
            char after_minus[MAX] = {0,}; //execution 2
            //after_comma => execution 3

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_day) && (atoi(after_minus) >= cur_day)) || ((atoi(after_comma) == cur_day) || (strchr(after_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_minus);
            // printf("2 %s\n", after_minus);
            // printf("3 %s\n", after_comma);
        }
        /* ex. 1, 15-30 */
        else if((strchr(before_comma, '-') == NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 2
            char after_minus[MAX] = {0,}; //execution 3
            //before_comma => execution 1

            strcpy(cpy_after_comma, after_comma);
            ptr2 = strtok(cpy_after_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_day) && (atoi(after_minus) >= cur_day)) || ((atoi(before_comma) == cur_day) || (strchr(before_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_comma);
            // printf("2 %s\n", before_minus);
            // printf("3 %s\n", after_minus);
        }     
    }
    /* 주기에 '-', '/' 포함 */
    else if ((strchr(day, '-') != NULL) && (strchr(day, ',') == NULL) && (strchr(day, '/') != NULL)) {
        char cpy_day[MAX] = {0,};
        char *ptr1;
        char *ptr2;
        char before_slash[MAX] = {0,};
        char after_slash[MAX] = {0,}; //execution 3
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2
        char cpy_before_slash[MAX] = {0,};

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_day, day);
        ptr1 = strtok(cpy_day, "/");
        strcpy(before_slash, ptr1);
        ptr1 = strtok(NULL, "/");
        strcpy(after_slash, ptr1);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash, before_slash);
        ptr2 = strtok(cpy_before_slash, "-");
        strcpy(before_minus, ptr2);
        ptr2 = strtok(NULL, "-");
        strcpy(after_minus, ptr2);

        int num = atoi(before_minus) % atoi(after_slash);
        if (((atoi(before_minus) <= cur_day) && (atoi(after_minus) >= cur_day) && (cur_day % atoi(after_slash) == num)))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
        // printf("3 %s\n", after_slash);
    }
    // /* 주기에 ',', '/' 포함 */
    // else if ((strchr(day, '-') == NULL) && (strchr(day, ',') != NULL) && (strchr(day, '/') != NULL)) {
    //     //에러처리
    // }

    /* 주기에 '-' 포함 */
    else if ((strchr(day, '-') != NULL) && (strchr(day, ',') == NULL) && (strchr(day, '/') == NULL)) {
        char cpy_day[MAX] = {0,};
        char *ptr;
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2

        strcpy(cpy_day, day);
        ptr = strtok(cpy_day, "-");
        strcpy(before_minus, ptr);
        ptr = strtok(NULL, "-");
        strcpy(after_minus, ptr);

        if ((atoi(before_minus) <= cur_day) && (atoi(after_minus) >= cur_day))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
    }
    /* 주기에 ',' 포함 */
    else if ((strchr(day, '-') == NULL) && (strchr(day, ',') != NULL) && (strchr(day, '/') == NULL)) {
        char cpy_day[MAX] = {0,};
        char *ptr;
        char before_comma[MAX] = {0,}; //execution 1
        char after_comma[MAX] = {0,}; //execution 2

        strcpy(cpy_day, day);
        ptr = strtok(cpy_day, ",");
        strcpy(before_comma, ptr);
        ptr = strtok(NULL, ",");
        strcpy(after_comma, ptr);

        if ((atoi(before_comma) == cur_day) || (atoi(after_comma) == cur_day) || (strchr(before_comma, '*') != NULL) || (strchr(after_comma, '*') != NULL))
            return 1;

        // printf("1 %s\n", before_comma);
        // printf("2 %s\n", after_comma);
    }
    /* 주기에 '/' 포함 */
    else if ((strchr(day, '-') == NULL) && (strchr(day, ',') == NULL) && (strchr(day, '/') != NULL)) {
        char cpy_day[MAX] = {0,};
        char *ptr;
        char before_slash[MAX] = {0,}; //execution 1
        char after_slash[MAX] = {0,}; //execution 2

        strcpy(cpy_day, day);
        ptr = strtok(cpy_day, "/");
        strcpy(before_slash, ptr);
        ptr = strtok(NULL, "/");
        strcpy(after_slash, ptr);

        if ((strchr(before_slash, '*') != NULL) && (cur_day % atoi(after_slash) == 1))
            return 1;

        // printf("1 %s\n", before_slash);
        // printf("2 %s\n", after_slash);
    }

    /* 주기에 '*' 혹은 숫자만 포함 */
    else {
        if((strchr(day, '*') != NULL) || (atoi(day) == cur_day))
            return 1;
    }

    return 0;
}


/**
함 수 : check_month(char month[])
기 능 : 현재의 날짜(월)와 비교하여 명령어 실행여부 결정
*/
int check_month(char month[]) {
    int cur_month;
    
    time_t t;
    struct tm tm;

    t = time(NULL);
    tm = *localtime(&t);
    cur_month = tm.tm_mon + 1;
    
    /* 주기에 '-', ',', '/' 포함 */
    if ((strchr(month, '-') != NULL) && (strchr(month, ',') != NULL) && (strchr(month, '/') != NULL)) {
        char cpy_month[MAX] = {0,};
	    char *ptr1;
        char *ptr2;
        char *ptr3;
        char *ptr4;
        char *ptr5;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};
        char before_slash1[MAX] = {0,};
        char after_slash1[MAX] = {0,}; //execution 3
        char before_slash2[MAX] = {0,};
        char after_slash2[MAX] = {0,}; //execution 6
        char before_minus1[MAX] = {0,}; //execution 1
        char after_minus1[MAX] = {0,}; //execution 2
        char before_minus2[MAX] = {0,}; //execution 4
        char after_minus2[MAX] = {0,}; //execution 5
        char cpy_before_comma[MAX] = {0,};
        char cpy_after_comma[MAX] = {0,};
        char cpy_before_slash1[MAX] = {0,};
        char cpy_before_slash2[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_month, month);
        ptr1 = strtok(cpy_month, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_before_comma, before_comma);
        ptr2 = strtok(cpy_before_comma, "/");
        strcpy(before_slash1, ptr2);
        ptr2 = strtok(NULL, "/");
        strcpy(after_slash1, ptr2);

        strcpy(cpy_after_comma, after_comma);
        ptr3 = strtok(cpy_after_comma, "/");
        strcpy(before_slash2, ptr3);
        ptr3 = strtok(NULL, "/");
        strcpy(after_slash2, ptr3);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash1, before_slash1);
        ptr4 = strtok(cpy_before_slash1, "-");
        strcpy(before_minus1, ptr4);
        ptr4 = strtok(NULL, "-");
        strcpy(after_minus1, ptr4);

        strcpy(cpy_before_slash2, before_slash2);
        ptr5 = strtok(cpy_before_slash2, "-");
        strcpy(before_minus2, ptr5);
        ptr5 = strtok(NULL, "-");
        strcpy(after_minus2, ptr5);

        int num1 = atoi(before_minus1) % atoi(after_slash1);
        int num2 = atoi(before_minus2) % atoi(after_slash2);
        if (((atoi(before_minus1) <= cur_month) && (atoi(after_minus1) >= cur_month) && (cur_month % atoi(after_slash1) == num1)) || ((atoi(before_minus2) <= cur_month) && (atoi(after_minus2) >= cur_month) && (cur_month % atoi(after_slash2) == num2)))
            return 1;

        // printf("1 %s\n", before_minus1);
        // printf("2 %s\n", after_minus1);
        // printf("3 %s\n", after_slash1);
        // printf("4 %s\n", before_minus2);
        // printf("5 %s\n", after_minus2);
        // printf("6 %s\n", after_slash2);
    }

    /* 주기에 '-', ',' 포함 */
    else if ((strchr(month, '-') != NULL) && (strchr(month, ',') != NULL) && (strchr(month, '/') == NULL)) {
        char cpy_month[MAX] = {0,};
        char *ptr1;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_month, month);
        ptr1 = strtok(cpy_month, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* ex. 1-3, 4-6 */
        if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char *ptr3;
            char before_minus1[MAX] = {0,}; //execution 1
            char after_minus1[MAX] = {0,}; //execution 2
            char before_minus2[MAX] = {0,}; //execution 3
            char after_minus2[MAX] = {0,}; //execution 4

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus1, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus1, ptr2);

            strcpy(cpy_after_comma, after_comma);
            ptr3 = strtok(cpy_after_comma, "-");
            strcpy(before_minus2, ptr3);
            ptr3 = strtok(NULL, "-");
            strcpy(after_minus2, ptr3);

            if (((atoi(before_minus1) <= cur_month) && (atoi(after_minus1) >= cur_month)) || ((atoi(before_minus2) <= cur_month) && (atoi(after_minus2) >= cur_month)))
            return 1;

            // printf("1 %s\n", before_minus1);
            // printf("2 %s\n", after_minus1);
            // printf("3 %s\n", before_minus2);
            // printf("4 %s\n", after_minus2);
        }
        /* ex. 1-3, 4 */
        else if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') == NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 1
            char after_minus[MAX] = {0,}; //execution 2
            //after_comma => execution 3

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_month) && (atoi(after_minus) >= cur_month)) || ((atoi(after_comma) == cur_month) || (strchr(after_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_minus);
            // printf("2 %s\n", after_minus);
            // printf("3 %s\n", after_comma);
        }
        /* ex. 1, 4-6 */
        else if((strchr(before_comma, '-') == NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 2
            char after_minus[MAX] = {0,}; //execution 3
            //before_comma => execution 1

            strcpy(cpy_after_comma, after_comma);
            ptr2 = strtok(cpy_after_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_month) && (atoi(after_minus) >= cur_month)) || ((atoi(before_comma) == cur_month) || (strchr(before_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_comma);
            // printf("2 %s\n", before_minus);
            // printf("3 %s\n", after_minus);
        }     
    }
    /* 주기에 '-', '/' 포함 */
    else if ((strchr(month, '-') != NULL) && (strchr(month, ',') == NULL) && (strchr(month, '/') != NULL)) {
        char cpy_month[MAX] = {0,};
        char *ptr1;
        char *ptr2;
        char before_slash[MAX] = {0,};
        char after_slash[MAX] = {0,}; //execution 3
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2
        char cpy_before_slash[MAX] = {0,};

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_month, month);
        ptr1 = strtok(cpy_month, "/");
        strcpy(before_slash, ptr1);
        ptr1 = strtok(NULL, "/");
        strcpy(after_slash, ptr1);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash, before_slash);
        ptr2 = strtok(cpy_before_slash, "-");
        strcpy(before_minus, ptr2);
        ptr2 = strtok(NULL, "-");
        strcpy(after_minus, ptr2);

        int num = atoi(before_minus) % atoi(after_slash);
        if (((atoi(before_minus) <= cur_month) && (atoi(after_minus) >= cur_month) && (cur_month % atoi(after_slash) == num)))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
        // printf("3 %s\n", after_slash);
    }
    // /* 주기에 ',', '/' 포함 */
    // else if ((strchr(month, '-') == NULL) && (strchr(month, ',') != NULL) && (strchr(month, '/') != NULL)) {
    //     //에러처리
    // }

    /* 주기에 '-' 포함 */
    else if ((strchr(month, '-') != NULL) && (strchr(month, ',') == NULL) && (strchr(month, '/') == NULL)) {
        char cpy_month[MAX] = {0,};
        char *ptr;
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2

        strcpy(cpy_month, month);
        ptr = strtok(cpy_month, "-");
        strcpy(before_minus, ptr);
        ptr = strtok(NULL, "-");
        strcpy(after_minus, ptr);

        if ((atoi(before_minus) <= cur_month) && (atoi(after_minus) >= cur_month))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
    }
    /* 주기에 ',' 포함 */
    else if ((strchr(month, '-') == NULL) && (strchr(month, ',') != NULL) && (strchr(month, '/') == NULL)) {
        char cpy_month[MAX] = {0,};
        char *ptr;
        char before_comma[MAX] = {0,}; //execution 1
        char after_comma[MAX] = {0,}; //execution 2

        strcpy(cpy_month, month);
        ptr = strtok(cpy_month, ",");
        strcpy(before_comma, ptr);
        ptr = strtok(NULL, ",");
        strcpy(after_comma, ptr);

        if ((atoi(before_comma) == cur_month) || (atoi(after_comma) == cur_month) || (strchr(before_comma, '*') != NULL) || (strchr(after_comma, '*') != NULL))
            return 1;

        // printf("1 %s\n", before_comma);
        // printf("2 %s\n", after_comma);
    }
    /* 주기에 '/' 포함 */
    else if ((strchr(month, '-') == NULL) && (strchr(month, ',') == NULL) && (strchr(month, '/') != NULL)) {
        char cpy_month[MAX] = {0,};
        char *ptr;
        char before_slash[MAX] = {0,}; //execution 1
        char after_slash[MAX] = {0,}; //execution 2

        strcpy(cpy_month, month);
        ptr = strtok(cpy_month, "/");
        strcpy(before_slash, ptr);
        ptr = strtok(NULL, "/");
        strcpy(after_slash, ptr);

        if ((strchr(before_slash, '*') != NULL) && (cur_month % atoi(after_slash) == 1))
            return 1;

        // printf("1 %s\n", before_slash);
        // printf("2 %s\n", after_slash);
    }

    /* 주기에 '*' 혹은 숫자만 포함 */
    else {
        if((strchr(month, '*') != NULL) || (atoi(month) == cur_month))
            return 1;
    }

    return 0;
}


/**
함 수 : check_dayofweek(char dayofweek[])
기 능 : 현재의 날짜(요일)와 비교하여 명령어 실행여부 결정
*/
int check_dayofweek(char dayofweek[]) {
    int cur_dayofweek;
    
    time_t t;
    struct tm tm;

    t = time(NULL);
    tm = *localtime(&t);
    cur_dayofweek = tm.tm_wday;
    
    /* 주기에 '-', ',', '/' 포함 */
    if ((strchr(dayofweek, '-') != NULL) && (strchr(dayofweek, ',') != NULL) && (strchr(dayofweek, '/') != NULL)) {
        char cpy_dayofweek[MAX] = {0,};
	    char *ptr1;
        char *ptr2;
        char *ptr3;
        char *ptr4;
        char *ptr5;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};
        char before_slash1[MAX] = {0,};
        char after_slash1[MAX] = {0,}; //execution 3
        char before_slash2[MAX] = {0,};
        char after_slash2[MAX] = {0,}; //execution 6
        char before_minus1[MAX] = {0,}; //execution 1
        char after_minus1[MAX] = {0,}; //execution 2
        char before_minus2[MAX] = {0,}; //execution 4
        char after_minus2[MAX] = {0,}; //execution 5
        char cpy_before_comma[MAX] = {0,};
        char cpy_after_comma[MAX] = {0,};
        char cpy_before_slash1[MAX] = {0,};
        char cpy_before_slash2[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_dayofweek, dayofweek);
        ptr1 = strtok(cpy_dayofweek, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_before_comma, before_comma);
        ptr2 = strtok(cpy_before_comma, "/");
        strcpy(before_slash1, ptr2);
        ptr2 = strtok(NULL, "/");
        strcpy(after_slash1, ptr2);

        strcpy(cpy_after_comma, after_comma);
        ptr3 = strtok(cpy_after_comma, "/");
        strcpy(before_slash2, ptr3);
        ptr3 = strtok(NULL, "/");
        strcpy(after_slash2, ptr3);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash1, before_slash1);
        ptr4 = strtok(cpy_before_slash1, "-");
        strcpy(before_minus1, ptr4);
        ptr4 = strtok(NULL, "-");
        strcpy(after_minus1, ptr4);

        strcpy(cpy_before_slash2, before_slash2);
        ptr5 = strtok(cpy_before_slash2, "-");
        strcpy(before_minus2, ptr5);
        ptr5 = strtok(NULL, "-");
        strcpy(after_minus2, ptr5);

        int num1 = atoi(before_minus1) % atoi(after_slash1);
        int num2 = atoi(before_minus2) % atoi(after_slash2);
        if (((atoi(before_minus1) <= cur_dayofweek) && (atoi(after_minus1) >= cur_dayofweek) && (cur_dayofweek % atoi(after_slash1) == num1)) || ((atoi(before_minus2) <= cur_dayofweek) && (atoi(after_minus2) >= cur_dayofweek) && (cur_dayofweek % atoi(after_slash2) == num2)))
            return 1;

        // printf("1 %s\n", before_minus1);
        // printf("2 %s\n", after_minus1);
        // printf("3 %s\n", after_slash1);
        // printf("4 %s\n", before_minus2);
        // printf("5 %s\n", after_minus2);
        // printf("6 %s\n", after_slash2);
    }

    /* 주기에 '-', ',' 포함 */
    else if ((strchr(dayofweek, '-') != NULL) && (strchr(dayofweek, ',') != NULL) && (strchr(dayofweek, '/') == NULL)) {
        char cpy_dayofweek[MAX] = {0,};
        char *ptr1;
        char before_comma[MAX] = {0,};
        char after_comma[MAX] = {0,};

        /* ','를 기준으로 문자열 분리 */
        strcpy(cpy_dayofweek, dayofweek);
        ptr1 = strtok(cpy_dayofweek, ",");
        strcpy(before_comma, ptr1);
        ptr1 = strtok(NULL, ",");
        strcpy(after_comma, ptr1);

        /* ex. 1-2, 4-5 */
        if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char *ptr3;
            char before_minus1[MAX] = {0,}; //execution 1
            char after_minus1[MAX] = {0,}; //execution 2
            char before_minus2[MAX] = {0,}; //execution 3
            char after_minus2[MAX] = {0,}; //execution 4

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus1, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus1, ptr2);

            strcpy(cpy_after_comma, after_comma);
            ptr3 = strtok(cpy_after_comma, "-");
            strcpy(before_minus2, ptr3);
            ptr3 = strtok(NULL, "-");
            strcpy(after_minus2, ptr3);

            if (((atoi(before_minus1) <= cur_dayofweek) && (atoi(after_minus1) >= cur_dayofweek)) || ((atoi(before_minus2) <= cur_dayofweek) && (atoi(after_minus2) >= cur_dayofweek)))
            return 1;

            // printf("1 %s\n", before_minus1);
            // printf("2 %s\n", after_minus1);
            // printf("3 %s\n", before_minus2);
            // printf("4 %s\n", after_minus2);
        }
        /* ex. 1-2, 4 */
        else if((strchr(before_comma, '-') != NULL) && (strchr(after_comma, '-') == NULL)) {
            char cpy_before_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 1
            char after_minus[MAX] = {0,}; //execution 2
            //after_comma => execution 3

            strcpy(cpy_before_comma, before_comma);
            ptr2 = strtok(cpy_before_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_dayofweek) && (atoi(after_minus) >= cur_dayofweek)) || ((atoi(after_comma) == cur_dayofweek) || (strchr(after_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_minus);
            // printf("2 %s\n", after_minus);
            // printf("3 %s\n", after_comma);
        }
        /* ex. 1, 4-5 */
        else if((strchr(before_comma, '-') == NULL) && (strchr(after_comma, '-') != NULL)) {
            char cpy_after_comma[MAX] = {0,};
            char *ptr2;
            char before_minus[MAX] = {0,}; //execution 2
            char after_minus[MAX] = {0,}; //execution 3
            //before_comma => execution 1

            strcpy(cpy_after_comma, after_comma);
            ptr2 = strtok(cpy_after_comma, "-");
            strcpy(before_minus, ptr2);
            ptr2 = strtok(NULL, "-");
            strcpy(after_minus, ptr2);

            if (((atoi(before_minus) <= cur_dayofweek) && (atoi(after_minus) >= cur_dayofweek)) || ((atoi(before_comma) == cur_dayofweek) || (strchr(before_comma, '*') != NULL)))
                return 1;

            // printf("1 %s\n", before_comma);
            // printf("2 %s\n", before_minus);
            // printf("3 %s\n", after_minus);
        }     
    }
    /* 주기에 '-', '/' 포함 */
    else if ((strchr(dayofweek, '-') != NULL) && (strchr(dayofweek, ',') == NULL) && (strchr(dayofweek, '/') != NULL)) {
        char cpy_dayofweek[MAX] = {0,};
        char *ptr1;
        char *ptr2;
        char before_slash[MAX] = {0,};
        char after_slash[MAX] = {0,}; //execution 3
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2
        char cpy_before_slash[MAX] = {0,};

        /* '/'를 기준으로 문자열 분리 */
        strcpy(cpy_dayofweek, dayofweek);
        ptr1 = strtok(cpy_dayofweek, "/");
        strcpy(before_slash, ptr1);
        ptr1 = strtok(NULL, "/");
        strcpy(after_slash, ptr1);

        /* '-'를 기준으로 문자열 분리 */
        strcpy(cpy_before_slash, before_slash);
        ptr2 = strtok(cpy_before_slash, "-");
        strcpy(before_minus, ptr2);
        ptr2 = strtok(NULL, "-");
        strcpy(after_minus, ptr2);

        int num = atoi(before_minus) % atoi(after_slash);
        if (((atoi(before_minus) <= cur_dayofweek) && (atoi(after_minus) >= cur_dayofweek) && (cur_dayofweek % atoi(after_slash) == num)))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
        // printf("3 %s\n", after_slash);
    }
    // /* 주기에 ',', '/' 포함 */
    // else if ((strchr(dayofweek, '-') == NULL) && (strchr(dayofweek, ',') != NULL) && (strchr(dayofweek, '/') != NULL)) {
    //     //에러처리
    // }

    /* 주기에 '-' 포함 */
    else if ((strchr(dayofweek, '-') != NULL) && (strchr(dayofweek, ',') == NULL) && (strchr(dayofweek, '/') == NULL)) {
        char cpy_dayofweek[MAX] = {0,};
        char *ptr;
        char before_minus[MAX] = {0,}; //execution 1
        char after_minus[MAX] = {0,}; //execution 2

        strcpy(cpy_dayofweek, dayofweek);
        ptr = strtok(cpy_dayofweek, "-");
        strcpy(before_minus, ptr);
        ptr = strtok(NULL, "-");
        strcpy(after_minus, ptr);

        if ((atoi(before_minus) <= cur_dayofweek) && (atoi(after_minus) >= cur_dayofweek))
            return 1;

        // printf("1 %s\n", before_minus);
        // printf("2 %s\n", after_minus);
    }
    /* 주기에 ',' 포함 */
    else if ((strchr(dayofweek, '-') == NULL) && (strchr(dayofweek, ',') != NULL) && (strchr(dayofweek, '/') == NULL)) {
        char cpy_dayofweek[MAX] = {0,};
        char *ptr;
        char before_comma[MAX] = {0,}; //execution 1
        char after_comma[MAX] = {0,}; //execution 2

        strcpy(cpy_dayofweek, dayofweek);
        ptr = strtok(cpy_dayofweek, ",");
        strcpy(before_comma, ptr);
        ptr = strtok(NULL, ",");
        strcpy(after_comma, ptr);

        if ((atoi(before_comma) == cur_dayofweek) || (atoi(after_comma) == cur_dayofweek) || (strchr(before_comma, '*') != NULL) || (strchr(after_comma, '*') != NULL))
            return 1;

        // printf("1 %s\n", before_comma);
        // printf("2 %s\n", after_comma);
    }
    /* 주기에 '/' 포함 */
    else if ((strchr(dayofweek, '-') == NULL) && (strchr(dayofweek, ',') == NULL) && (strchr(dayofweek, '/') != NULL)) {
        char cpy_dayofweek[MAX] = {0,};
        char *ptr;
        char before_slash[MAX] = {0,}; //execution 1
        char after_slash[MAX] = {0,}; //execution 2

        strcpy(cpy_dayofweek, dayofweek);
        ptr = strtok(cpy_dayofweek, "/");
        strcpy(before_slash, ptr);
        ptr = strtok(NULL, "/");
        strcpy(after_slash, ptr);

        if ((strchr(before_slash, '*') != NULL) && (cur_dayofweek % atoi(after_slash) == 0))
            return 1;

        // printf("1 %s\n", before_slash);
        // printf("2 %s\n", after_slash);
    }

    /* 주기에 '*' 혹은 숫자만 포함 */
    else {
        if((strchr(dayofweek, '*') != NULL) || (atoi(dayofweek) == cur_dayofweek))
            return 1;
    }

    return 0;
}