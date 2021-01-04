#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define MAX 1024
#define SECOND_TO_MICRO 1000000

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t); /* ssu_mntr의 수행시간 측정 */

int check_execution_cycle(char *execution_cycle_1, char *execution_cycle_2, char *execution_cycle_3, char *execution_cycle_4, char *execution_cycle_5); /* 실행주기가 규칙에 맞는지 확인 */
void cmd_add(char execution_cycle[], char *instructions); /* ssu_crontab_file에 입력받은 명령어 추가 */
void add_log(char performance_time[], char execution_cycle[], char *instructions); /* ssu_crontab_file에 명령어가 추가됐을 경우, log 남김 */
void cmd_remove(char *command_number); /* ssu_crontab_file에서 입력받은 번호의 명령어 삭제 */
void remove_log(char performance_time[], char execution_cycle[], char instructions[]); /* ssu_crontab_file에 명령어가 삭제됐을 경우, log 남김 */


int main()
{
    char buf[MAX]={0,};
    char *cpy;
    char *cmd;
    char *command_number;
    char *execution_cycle_1;
    char *execution_cycle_2;
    char *execution_cycle_3;
    char *execution_cycle_4;
    char *execution_cycle_5;
    char execution_cycle[MAX]={0,};
    char *instructions;
    char file_contents[MAX]={0,};

    struct timeval begin_t, end_t;
    FILE *fp;

    gettimeofday(&begin_t, NULL);


    while (1) { //무한 반복하며 명령어 입력받음
        if(access("ssu_crontab_file", F_OK) == 0) {
            if((fp = fopen("ssu_crontab_file", "a+")) == NULL) { //info에서 정보 가져옴
                perror("fopen error\n");
                exit(1);
            }

            while(fgets(file_contents, MAX, fp) != NULL)
            printf("%s", file_contents);

            fclose(fp);
        }

        
        printf("\n20180737>"); //프롬프트 출력
        fgets(buf, MAX, stdin);
        cpy = malloc(strlen(buf)+1);
        strcpy(cpy, buf);
        cmd = strtok(cpy, " ");
        execution_cycle_1 = strtok(NULL, " ");
        command_number = execution_cycle_1;
        execution_cycle_2 = strtok(NULL, " ");
        execution_cycle_3 = strtok(NULL, " ");
        execution_cycle_4 = strtok(NULL, " ");
        execution_cycle_5 = strtok(NULL, " ");
        sprintf(execution_cycle, "%s %s %s %s %s", execution_cycle_1, execution_cycle_2, execution_cycle_3, execution_cycle_4, execution_cycle_5);

        instructions = strtok(NULL, "\n");
        
        if(strstr(cmd, "add") != NULL) {
            if (check_execution_cycle(execution_cycle_1, execution_cycle_2, execution_cycle_3, execution_cycle_4, execution_cycle_5) == 1)
                continue;

            cmd_add(execution_cycle, instructions);
        }

        else if(strstr(cmd, "remove") != NULL) {
            cmd_remove(command_number);
        }

        else if(strstr(cmd, "exit") != NULL) {
            printf("프로그램 종료\n");

            gettimeofday(&end_t, NULL);
            ssu_runtime(&begin_t, &end_t);

            exit(0);
        }

        else if(strcmp(cmd, "\n") == 0)
            continue;

        else {
            printf("실행 가능한 명령어가 아닙니다.\n");
            continue;
        }


        free(cpy);
        cpy = NULL;
    }

    exit(0);
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


/**
함 수 : cmd_add(char execution_cycle[], char *instructions)
기 능 : ssu_crontab_file에 입력받은 명령어 추가
*/
void cmd_add(char execution_cycle[], char *instructions) {
    
    char file_contents[MAX]={0,};
    int file_contents_num = 0;
    char ssu_crontab_file_absolute_path[MAX]={0,};
    char performance_time[MAX]={0,};

    FILE *fp;
    struct stat ssu_crontab_file;
    
    if((fp = fopen("ssu_crontab_file", "a+")) == NULL) { //ssu_crontab_file open (파일이 없으면 생성)
        perror("fopen error\n");
        exit(1);
    }

    while(fgets(file_contents, MAX, fp) != NULL)
        file_contents_num++;


    memset(file_contents, 0, MAX);

    sprintf(file_contents, "%d. %s %s", file_contents_num, execution_cycle, instructions);
    fprintf(fp, "%s\n", file_contents);

    fclose(fp);


    if(realpath("ssu_crontab_file", ssu_crontab_file_absolute_path) == NULL) {
        perror("realpath error\n");
        exit(1);
    }

    lstat(ssu_crontab_file_absolute_path, &ssu_crontab_file);
    sprintf(performance_time, "%s", ctime(&ssu_crontab_file.st_mtime));
    performance_time[strlen(performance_time) - 1] = '\0'; //개행문자 제거

    add_log(performance_time, execution_cycle, instructions);

}


/**
함 수 : check_execution_cycle(char *execution_cycle_1, char *execution_cycle_2, char *execution_cycle_3, char *execution_cycle_4, char *execution_cycle_5)
기 능 : 실행주기가 규칙에 맞게 입력되었는지 확인
*/
int check_execution_cycle(char *execution_cycle_1, char *execution_cycle_2, char *execution_cycle_3, char *execution_cycle_4, char *execution_cycle_5) {

    /* 실행주기의 항목이 5개인지 확인 */
    if (execution_cycle_1 == NULL)
        return 1;

    if (execution_cycle_2 == NULL)
        return 1;

    if (execution_cycle_3 == NULL)
        return 1;

    if (execution_cycle_4 == NULL)
        return 1;

    if (execution_cycle_5 == NULL)
        return 1;


    /* 각 항목이 범위를 벗어나는지 확인 
       '/' 기호가 나오는 경우에 '주기/숫자'의 형태인지 확인 
       '-' 기호가 나오는 경우에  앞의 숫자 < 뒤의 숫자인지 확인 */
    
    /* 분 */
    if(strchr(execution_cycle_1, '-') != NULL) {
        char *cpy_execution_cycle_1 = malloc(strlen(execution_cycle_1)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_1, execution_cycle_1);
        num1 = strtok(cpy_execution_cycle_1, "-");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "-");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 0) || (atoi(num1) > 59))
            return 1;
        if ((atoi(num2) < 0) || (atoi(num2) > 59))
            return 1;
        if(atoi(num1) > atoi(num2))
            return 1;
        
        free(cpy_execution_cycle_1);
        cpy_execution_cycle_1 = NULL;
    }
    else if(strchr(execution_cycle_1, ',') != NULL) {
        char *cpy_execution_cycle_1 = malloc(strlen(execution_cycle_1)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_1, execution_cycle_1);
        num1 = strtok(cpy_execution_cycle_1, ",");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, ",");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 0) || (atoi(num1) > 59))
            return 1;
        if ((atoi(num2) < 0) || (atoi(num2) > 59))
            return 1;

        free(cpy_execution_cycle_1);
        cpy_execution_cycle_1 = NULL;
    }
    else if(strchr(execution_cycle_1, '/') != NULL) {
        char *cpy_execution_cycle_1 = malloc(strlen(execution_cycle_1)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_1, execution_cycle_1);
        num1 = strtok(cpy_execution_cycle_1, "/");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "/");
        if (num2 == NULL)
            return 1;

        if ((atoi(num2) < 0) || (atoi(num2) > 59))
            return 1;

        free(cpy_execution_cycle_1);
        cpy_execution_cycle_1 = NULL;
    }
    else {
        if(strchr(execution_cycle_1, '*') == NULL) {
            if ((atoi(execution_cycle_1) < 0) || (atoi(execution_cycle_1) > 59))
                return 1;
        }
    }


    /* 시 */
    if(strchr(execution_cycle_2, '-') != NULL) {
        char *cpy_execution_cycle_2 = malloc(strlen(execution_cycle_2)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_2, execution_cycle_2);
        num1 = strtok(cpy_execution_cycle_2, "-");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "-");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 0) || (atoi(num1) > 23))
            return 1;
        if ((atoi(num2) < 0) || (atoi(num2) > 23))
            return 1;
        if(atoi(num1) > atoi(num2))
            return 1;

        free(cpy_execution_cycle_2);
        cpy_execution_cycle_2 = NULL;
    }
    else if(strchr(execution_cycle_2, ',') != NULL) {
        char *cpy_execution_cycle_2 = malloc(strlen(execution_cycle_2)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_2, execution_cycle_2);
        num1 = strtok(cpy_execution_cycle_2, ",");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, ",");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 0) || (atoi(num1) > 23))
            return 1;
        if ((atoi(num2) < 0) || (atoi(num2) > 23))
            return 1;

        free(cpy_execution_cycle_2);
        cpy_execution_cycle_2 = NULL;
    }
    else if(strchr(execution_cycle_2, '/') != NULL) {
        char *cpy_execution_cycle_2 = malloc(strlen(execution_cycle_2)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_2, execution_cycle_2);
        num1 = strtok(cpy_execution_cycle_2, "/");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "/");
        if (num2 == NULL)
            return 1;

        if ((atoi(num2) < 0) || (atoi(num2) > 23))
            return 1;

        free(cpy_execution_cycle_2);
        cpy_execution_cycle_2 = NULL;
    }
    else {
        if(strchr(execution_cycle_2, '*') == NULL) {
            if ((atoi(execution_cycle_2) < 0) || (atoi(execution_cycle_2) > 23))
                return 1;
        }
    }


    /* 일 */
    if(strchr(execution_cycle_3, '-') != NULL) {
        char *cpy_execution_cycle_3 = malloc(strlen(execution_cycle_3)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_3, execution_cycle_3);
        num1 = strtok(cpy_execution_cycle_3, "-");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "-");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 1) || (atoi(num1) > 31))
            return 1;
        if ((atoi(num2) < 1) || (atoi(num2) > 31))
            return 1;
        if(atoi(num1) > atoi(num2))
            return 1;

        free(cpy_execution_cycle_3);
        cpy_execution_cycle_3 = NULL;
    }
    else if(strchr(execution_cycle_3, ',') != NULL) {
        char *cpy_execution_cycle_3 = malloc(strlen(execution_cycle_3)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_3, execution_cycle_3);
        num1 = strtok(cpy_execution_cycle_3, ",");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, ",");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 1) || (atoi(num1) > 31))
            return 1;
        if ((atoi(num2) < 1) || (atoi(num2) > 31))
            return 1;

        free(cpy_execution_cycle_3);
        cpy_execution_cycle_3 = NULL;
    }
    else if(strchr(execution_cycle_3, '/') != NULL) {
        char *cpy_execution_cycle_3 = malloc(strlen(execution_cycle_3)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_3, execution_cycle_3);
        num1 = strtok(cpy_execution_cycle_3, "/");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "/");
        if (num2 == NULL)
            return 1;

        if ((atoi(num2) < 1) || (atoi(num2) > 31))
            return 1;

        free(cpy_execution_cycle_3);
        cpy_execution_cycle_3 = NULL;
    }
    else {
        if(strchr(execution_cycle_3, '*') == NULL) {
            if ((atoi(execution_cycle_3) < 1) || (atoi(execution_cycle_3) > 31))
                return 1;
        }
    }


    /* 월 */
    if(strchr(execution_cycle_4, '-') != NULL) {
        char *cpy_execution_cycle_4 = malloc(strlen(execution_cycle_4)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_4, execution_cycle_4);
        num1 = strtok(cpy_execution_cycle_4, "-");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "-");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 1) || (atoi(num1) > 12))
            return 1;
        if ((atoi(num2) < 1) || (atoi(num2) > 12))
            return 1;
        if(atoi(num1) > atoi(num2))
            return 1;

        free(cpy_execution_cycle_4);
        cpy_execution_cycle_4 = NULL;
    }
    else if(strchr(execution_cycle_4, ',') != NULL) {
        char *cpy_execution_cycle_4 = malloc(strlen(execution_cycle_4)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_4, execution_cycle_4);
        num1 = strtok(cpy_execution_cycle_4, ",");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, ",");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 1) || (atoi(num1) > 12))
            return 1;
        if ((atoi(num2) < 1) || (atoi(num2) > 12))
            return 1;

        free(cpy_execution_cycle_4);
        cpy_execution_cycle_4 = NULL;
    }
    else if(strchr(execution_cycle_4, '/') != NULL) {
        char *cpy_execution_cycle_4 = malloc(strlen(execution_cycle_4)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_4, execution_cycle_4);
        num1 = strtok(cpy_execution_cycle_4, "/");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "/");
        if (num2 == NULL)
            return 1;

        if ((atoi(num2) < 1) || (atoi(num2) > 12))
            return 1;

        free(cpy_execution_cycle_4);
        cpy_execution_cycle_4 = NULL;
    }
    else {
        if(strchr(execution_cycle_4, '*') == NULL) {
            if ((atoi(execution_cycle_4) < 1) || (atoi(execution_cycle_4) > 12))
                return 1;
        }
    }


    /* 요일 */
    if(strchr(execution_cycle_5, '-') != NULL) {
        char *cpy_execution_cycle_5 = malloc(strlen(execution_cycle_5)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_5, execution_cycle_5);
        num1 = strtok(cpy_execution_cycle_5, "-");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "-");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 0) || (atoi(num1) > 6))
            return 1;
        if ((atoi(num2) < 0) || (atoi(num2) > 6))
            return 1;
        if(atoi(num1) > atoi(num2))
            return 1;

        free(cpy_execution_cycle_5);
        cpy_execution_cycle_5 = NULL;
    }
    else if(strchr(execution_cycle_5, ',') != NULL) {
        char *cpy_execution_cycle_5 = malloc(strlen(execution_cycle_5)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_5, execution_cycle_5);
        num1 = strtok(cpy_execution_cycle_5, ",");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, ",");
        if (num2 == NULL)
            return 1;

        if ((atoi(num1) < 0) || (atoi(num1) > 6))
            return 1;
        if ((atoi(num2) < 0) || (atoi(num2) > 6))
            return 1;

        free(cpy_execution_cycle_5);
        cpy_execution_cycle_5 = NULL;
    }
    else if(strchr(execution_cycle_5, '/') != NULL) {
        char *cpy_execution_cycle_5 = malloc(strlen(execution_cycle_5)+1);
        char *num1;
        char *num2;

        strcpy(cpy_execution_cycle_5, execution_cycle_5);
        num1 = strtok(cpy_execution_cycle_5, "/");
        if (num1 == NULL)
            return 1;
        num2 = strtok(NULL, "/");
        if (num2 == NULL)
            return 1;

        if ((atoi(num2) < 0) || (atoi(num2) > 6))
            return 1;

        free(cpy_execution_cycle_5);
        cpy_execution_cycle_5 = NULL;
    }
    else {
        if(strchr(execution_cycle_5, '*') == NULL) {
            if ((atoi(execution_cycle_5) < 0) || (atoi(execution_cycle_5) > 6))
                return 1;
        }
    }


    /* '*', '-', ',', '/' 이외의 기호가 나온 경우 */
    char key[] = "*-,/0123456789";

    if(strpbrk(execution_cycle_1, key) == NULL)
        return 1;
    if(strpbrk(execution_cycle_2, key) == NULL)
        return 1;
    if(strpbrk(execution_cycle_3, key) == NULL)
        return 1;
    if(strpbrk(execution_cycle_4, key) == NULL)
        return 1;
    if(strpbrk(execution_cycle_5, key) == NULL)
        return 1;

    return 0;
}


/**
함 수 : add_log(char performance_time[], char execution_cycle[], char *instructions)
기 능 : ssu_crontab_file에 명령어를 추가했을 경우, log 남김
*/
void add_log(char performance_time[], char execution_cycle[], char *instructions) {

    char log_contents[MAX]={0,};
    
    FILE *fp;

    if((fp = fopen("ssu_crontab_log", "a+")) == NULL) { //ssu_crontab_log open (파일이 없으면 생성)
        perror("fopen error\n");
        exit(1);
    }

    sprintf(log_contents, "[%s] add %s %s", performance_time, execution_cycle, instructions);
    fprintf(fp, "%s\n", log_contents);

    fclose(fp);

}


/**
함 수 : cmd_remove(char *command_number)
기 능 : ssu_crontab_file에서 인자로 입력받은 번호의 명령어를 삭제
*/
void cmd_remove(char *command_number) {

    char file_contents[MAX]={0,};
    int file_contents_num = 0;
    int integer_command_number = 0;
    char ssu_crontab_file_absolute_path[MAX]={0,};
    char performance_time[MAX]={0,};
    char execution_cycle[MAX]={0,};
    char instructions[MAX]={0,};
    
    FILE *fp;
    FILE *fp2;
    struct stat ssu_crontab_file;

    if(command_number == NULL) {
        printf("COMMAND_NUMBER의 입력이 없습니다.\n");
        return;
    }

    integer_command_number = atoi(command_number);

    if((fp = fopen("ssu_crontab_file", "r")) == NULL) { //ssu_crontab_file open
        perror("fopen error\n");
        exit(1);
    }

    while(fgets(file_contents, MAX, fp) != NULL)
        file_contents_num++;
    
    fclose(fp);

    if(integer_command_number >= file_contents_num) {
        file_contents_num = 0;
        printf("잘못된 COMMAND_NUMBER입니다.\n");
        return;
    }
    file_contents_num = 0;


    if((fp = fopen("ssu_crontab_file", "r")) == NULL) { //ssu_crontab_file open
        perror("fopen error\n");
        exit(1);
    }

    if((fp2 = fopen("remove_ssu_crontab_file", "w+")) == NULL) { //해당 내용을 지운 ssu_crontab_file open
        perror("fopen error\n");
        exit(1);
    }

    while(fgets(file_contents, MAX, fp) != NULL) {
        if(file_contents_num < integer_command_number) {
            fputs(file_contents, fp2);
        }

        else if(file_contents_num == integer_command_number) {
            char cpy_file_contents[MAX];
	        char *ptr;

            strcpy(cpy_file_contents, file_contents);
            strtok(cpy_file_contents, " ");
            ptr = strtok(NULL, " ");
            strcpy(execution_cycle, ptr);
            ptr = strtok(NULL, " ");
            strcat(execution_cycle, " ");
            strcat(execution_cycle, ptr);
            ptr = strtok(NULL, " ");
            strcat(execution_cycle, " ");
            strcat(execution_cycle, ptr);
            ptr = strtok(NULL, " ");
            strcat(execution_cycle, " ");
            strcat(execution_cycle, ptr);
            ptr = strtok(NULL, " ");
            strcat(execution_cycle, " ");
            strcat(execution_cycle, ptr);

            ptr = strtok(NULL, "\0");
            strcpy(instructions, ptr);
        }

        else if(file_contents_num > integer_command_number) {
            char cpy_file_contents[MAX];
	        char *ptr;
            int integer_behind_command_number;
            char new_file_contents[MAX];

        	strcpy(cpy_file_contents, file_contents);
            ptr = strtok(cpy_file_contents, ".");
            integer_behind_command_number = atoi(ptr);
            ptr = strtok(NULL, "\0");
            sprintf(new_file_contents, "%d.%s", integer_behind_command_number-1, ptr);

            fputs(new_file_contents, fp2);
        }

        file_contents_num++;
        memset(file_contents, 0, MAX);
    }

    fclose(fp);
    fclose(fp2);

    if(rename("remove_ssu_crontab_file", "ssu_crontab_file") == -1) {
        fprintf(stderr, "rename error\n");
        exit(1);
    }


    if(realpath("ssu_crontab_file", ssu_crontab_file_absolute_path) == NULL) {
        perror("realpath error\n");
        exit(1);
    }

    lstat(ssu_crontab_file_absolute_path, &ssu_crontab_file);
    sprintf(performance_time, "%s", ctime(&ssu_crontab_file.st_mtime));
    performance_time[strlen(performance_time) - 1] = '\0'; //개행문자 제거

    remove_log(performance_time, execution_cycle, instructions);
}


/**
함 수 : remove_log(char performance_time[], char execution_cycle[], char instructions[])
기 능 : ssu_crontab_file에서 명령어를 삭제했을 경우에 log 남김
*/
void remove_log(char performance_time[], char execution_cycle[], char instructions[]) {
    char log_contents[MAX]={0,};
    
    FILE *fp;

    if((fp = fopen("ssu_crontab_log", "a+")) == NULL) { //ssu_crontab_log open (파일이 없으면 생성)
        perror("fopen error\n");
        exit(1);
    }

    sprintf(log_contents, "[%s] remove %s %s", performance_time, execution_cycle, instructions);
    fprintf(fp, "%s\n", log_contents);

    fclose(fp);
}