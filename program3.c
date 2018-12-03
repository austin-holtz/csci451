//Austin Holtz

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define PIPE_EMPTY 0
#define PIPE_FULL 1

/*
    accept cli inputs: pipe id, sem key, shared mem key, output file name
    read from second pipe, reconstruct original document, print to output file
    use semaphore to sync the pipe (1 word at a time)
    once all words have been read, display values from shared memory
    terminate
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

    //get cli input
    char* output_file_name = (char*) argv[0];
    int pipe_2_read = atoi(argv[1]);
    key_t sem_key = atoi(argv[2]);
    key_t shm_key = atoi(argv[3]);

    //get semaphores
    int sem_id = semget(sem_key, 0, 0);
    if (sem_id == -1){
        perror("Program 2: error getting semaphore 1 id");
        exit(1);
    }

    //get out file desc
    FILE* fp = fopen(output_file_name, "w");

    int sem_val;
    char* input_buffer = calloc(50, 1);
    while(1){
        printf("p3 entered while loop\n");
        while(semctl(sem_id, 0, GETVAL)==PIPE_EMPTY); //wait
        if(read(pipe_2_read, input_buffer, 50)==-1){
            perror("p3 reading pipe");
        }
        if(strlen(input_buffer)==0) break;
        fprintf(fp, "%s ", input_buffer);
        sem_ctl_union.val=PIPE_EMPTY;
        semctl(sem_id, 0, SETVAL, sem_ctl_union);
    }
    close(pipe_2_read);
    fclose(fp);
    semctl(sem_id, 0, IPC_RMID);

    //access shared memory and print values
    int shm_id = shmget(shm_key, 0, 0);
    int* shm_ptr = shmat(shm_id, NULL, 0);
    printf("Type 1: %d\n", shm_ptr[0]);
    printf("Type 2: %d\n", shm_ptr[1]);

    //detatch shared memory
    if(shmdt(shm_ptr)==-1){
        perror("deleting memory");
    }
    return 0;
}
