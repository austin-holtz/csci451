//Austin Holtz

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define PIPE_EMPTY 0
#define PIPE_FULL 1

/*

Accept cli inputs from master
read input text file, extract words.
write 1 word per line to the pipe
Use a semaphore to ensure that there is only one word in the pipe at a time
when all words are passed terminate *close the pipe*

*/ 

int main(int argc, char const *argv[])
{
    //initialze sem ctl union
    union semun
    {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
        struct seminfo *__buf;
    } sem_ctl_union;

    //gather cli input
    char* input_file_name = (char*) argv[0];
    int pipe_write_id = (int) atoi(argv[1]);
    const key_t sem_key = (int) atoi(argv[2]);

    //retrieve semaphore
    const int sem_id = semget(sem_key, 0, 0);
    if (sem_id == -1){
        perror("Program 1: error retrieving semaphore");
        exit(0);
    }

    //open input file
    FILE *fp = fopen(input_file_name, "r");
    

    //pass words into pipe
    
    int pipe_status;
    char* input_buffer = calloc(50,sizeof(char));
    while(fscanf(fp, "%s ", input_buffer)==1){
        while(semctl(sem_id, 0, GETVAL)==PIPE_FULL);//wait

        write(pipe_write_id, input_buffer, strlen(input_buffer)+1);
        // printf("p1: %s\n", input_buffer);

        sem_ctl_union.val=PIPE_FULL;
        semctl(sem_id, 0, SETVAL, sem_ctl_union);
    }

    while(semctl(sem_id, 0, GETVAL)==PIPE_FULL); //wait
    write(pipe_write_id, "\0", 1);
    sem_ctl_union.val=1;
    semctl(sem_id, 0, SETVAL, sem_ctl_union);

    close(pipe_write_id);
    
    return(0);
}
