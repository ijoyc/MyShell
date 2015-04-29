//
//  main.c
//  MyShell
//
//  Created by Chen Yizhuo on 15/4/29.
//  Copyright (c) 2015年 Chen Yizhuo. All rights reserved.
//

#include <stdio.h>
#include <string.h> //strcmp, strtok
#include <unistd.h> //getpid, chdir, _exit
#include <signal.h> //kill
#include <errno.h>  //errno, EINTR
#include <stdlib.h> //EXIT_FAILURE

#define MAX_SIZE 100

#pragma mark - Global Variables

char line[MAX_SIZE];    //get user input from commond line
int flag;   //support relocation or not? ie. ls > result.txt
int back_flag;  //run the progress in the back?

#pragma mark - Functions Declaration

/**
 *  exit terminal
 */
void my_exit();

/**
 *  commond "cd", go to target directory
 *
 *  @param target target directory
 */
void my_chdir(char *target);

/**
 *  other commonds. Let linux to run it.
 */
void my_unix(char *commond);


#pragma mark - Main

int main(int argc, const char * argv[]) {
    
    //default no need to relocation
    flag = 0;
    //default not in the back
    back_flag = 0;
    
    char *commond;
    
    
    
    while (1) {
        //#warning Not available in MacOS
//        char *userName = getlogin(); //user name
//        char *path = get_current_dir_name(); //currentPath
        //format: user@user-desktop:~path
//        printf("%s@%s-desktop:~%s$", userName, userName, path);
        
        fgets(line, MAX_SIZE, stdin);   //read from stdin
        //split user's input and get the first "string"
        //here use " ", "\n", "\t"... as the delimiter
        commond = strtok(line, " \n\t();");
        if (commond == NULL) {
            continue;       //if user input nothing, then ingore it.
        } else if (strcmp(commond, "exit") == 0) {
            //if user input "exit" the exit the application.
            my_exit();
        } else if (strcmp(commond, "cd") == 0) {
            my_chdir(commond);
        } else {
            //let system handle other commonds
            my_unix(commond);
        }
    }
    
    return 0;
}


#pragma mark - Other Functions

void my_exit() {
    //    printf("exit");
    pid_t pid = getpid();
    //SIGTERM: software termination signal from kill
    kill(pid, SIGTERM);
}

void my_chdir(char *target) {
    //    printf("cd");
    
    int status;
    
    //in order to get next token and to continue with the same string.
    //NULL is passed as first argument
    target = strtok(NULL, " \n\t();");
    while (target) {
        status = chdir(target);
        //handle error
        if (status < 0) {
            fprintf(stderr, "Error in my_chdir(%s) : %s\n", target, strerror(errno));
            return;
        }
        target = strtok(NULL, " \n\t();");
    }
}

void my_unix(char *commond) {
    //    printf("other");
    
    pid_t pid;
    char *args[25];
    int count = 0;
    FILE *file; //the destination of relocation
    int execvp_status;  //execvp success or not?
    int wait_status;    //wait success or not?
    int close_status;   //close file success or not?
    int wait_arg;       //the arg of wait();
    
    //add first part of commond to args.
    args[0] = commond;
    
    commond = strtok(NULL, " \n\t();");
    while (commond != NULL) {
        
        if (strcmp(commond, ">") == 0) {
            flag = 1;   //relocation
        } else if (strcmp(commond, "&") == 0) {
            back_flag = 1;
            commond = strtok(NULL, " \n\t();");
        }
        
        count++;
        args[count] = commond;
        commond = strtok(NULL, " \n\t();");
    }
    
    //the end of commond
    count++;
    args[count] = NULL;
    
    pid = fork();
    //handle error
    if (pid < 0) {
        fprintf(stderr, "Error in my_unix(%s) when fork child progress : %s", args[0], strerror(errno));
        _exit(0);
    }
    
    if (pid == 0) {
        //child progress
        
        //psu-relocate, just put commonds to file.
        if (flag == 1) {
            //args[count - 1] is the last argument, which represent the file to relocate
            file = freopen(args[count - 1], "w+", stdout);
            if (file == NULL) {
                fprintf(stderr, "Error in my_unix(%s) when reopen file : %s", args[0], strerror(errno));
                _exit(0);
            }
            
            int i = 0;
            while (args[i] != NULL) {
                fprintf(file, "%s", args[i]);
                i++;
            }
            
            //cleanup
            close_status = fclose(file);
            if (close_status != 0) {
                fprintf(stderr, "Error in my_unix(%s) when close file : %s", args[0], strerror(errno));
                _exit(EXIT_FAILURE);
            }
            flag = 0;
            back_flag = 0;
            _exit(0);
        }
        
        execvp_status = execvp(args[0], args);
        if (execvp_status < 0) {
            fprintf(stderr, "Error in my_unix(%s) when execvp : %s", args[0], strerror(errno));
            _exit(EXIT_FAILURE);
        }
        
        _exit(0);
    } else {
        //parent progress
        
        //if child progress is in back, then parent is not necessary to wait.
        if (back_flag == 0) {
            wait_status = wait(&wait_arg);
            if (errno == EINTR) {
                return;
            }
            if (wait_status < 0) {
                fprintf(stderr, "Error in my_unix(%s) with wait() : %s", args[0], strerror(errno));
                _exit(EXIT_FAILURE);
            }
            
            flag = 0;
            back_flag = 0;
        } else {
            printf("Pid = %d\n", getpid());
        }
    }
}

