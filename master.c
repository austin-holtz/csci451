//Austin Holtz

#include <stdlib.h>
#include <stdio.h>

#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define PIPE_EMPTY 0
#define PIPE_FULL 1

//defining pipe ends because it seems counterintuitive
#define PIPE_READ_INDEX 0
#define PIPE_WRITE_INDEX 1

/*
Accepts two cli inputs:
    text file to read
    output file

Create two pipes, 2 semaphores, and 1 shared memory segment.
Fork program1 and pass the following via args:
    input filename
    write side of pipe1
    semaphore1

fork program2 and pass:
    read side pipe1
    write side pipe2
    semaphore1
    semaphore2
    shared memory

fork program3 and pass:
    output filename
    read side of pipe2
    semaphore2
    shared memory

terminate once the other programs do
*/


int main(int argc, char const *argv[])
{
    //gather args
    const char *input_file_name = argv[1];
    const char *output_file_name = argv[2];
    
    //create semaphores
    key_t sem_1_key = ftok("./key", 1);
    key_t sem_2_key = ftok("./key", 2);
    int sem_1_id = semget(sem_1_key, 1, 0666|IPC_CREAT);
    if (sem_1_id == -1){
        perror("Error creating semaphore 1");
    }

    int sem_2_id = semget(sem_2_key, 1, 0666|IPC_CREAT);
    if (sem_2_id == -1){
        perror("Error creating semaphore 2");
    }


    union semun
    {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
        struct seminfo *__buf;
    } sem_ctl_union;

    struct semid_ds sem_ctl_struct;

    // sem_ctl_struct.sem_perm.mode = 0666;
    sem_ctl_union.buf = &sem_ctl_struct;

    // if(semctl(sem_1_id, 0, IPC_SET, sem_ctl_union)==-1){
    //     perror("Error setting sem permissions");
    // }

    sem_ctl_union.val = 0;
    if(semctl(sem_1_id, 0, SETVAL, sem_ctl_union)==-1){
        perror("Error setting sem value to 0");
    }
    if(semctl(sem_2_id, 0, SETVAL, sem_ctl_union)==-1){
        perror("Error setting sem value to 0");
    }

    //create pipes
    int pipe1fd[2];
    int pipe2fd[2];
    if (pipe(pipe1fd)==-1){
        perror("Error creating pipe1");
    }
    if (pipe(pipe2fd)==-1){
        perror("Error creating pipe1");
    }

    //create shared memory
    key_t shm_key = ftok("./key", 3);
    int shm_id = shmget(shm_key, sizeof(int)*2, 0666|IPC_CREAT);
    if (shm_id == -1){
        perror("Error creating shared memory");
        exit(1);
    }

    //create arguments for fork commands
    //semaphores
    char sem_1_key_str[20];
    char sem_2_key_str[20];
    sprintf(sem_1_key_str, "%d", sem_1_key);
    sprintf(sem_2_key_str, "%d", sem_2_key);

    //pipes
    char pipe_1_write_str[10];
    char pipe_1_read_str[10];
    char pipe_2_write_str[10];
    char pipe_2_read_str[10];
    sprintf(pipe_1_write_str, "%d", pipe1fd[PIPE_WRITE_INDEX]);
    sprintf(pipe_1_read_str, "%d", pipe1fd[PIPE_READ_INDEX]);
    sprintf(pipe_2_write_str, "%d", pipe2fd[PIPE_WRITE_INDEX]);
    sprintf(pipe_2_read_str, "%d", pipe2fd[PIPE_READ_INDEX]);

    //shared memory
    char shm_key_str[20];
    sprintf(shm_key_str, "%d", shm_key);

    //initialize fork PID storage
    pid_t p1_pid, p2_pid, p3_pid;

    // fork program1 and pass data
    pid_t current_pid = fork();
    if (current_pid == -1){
        perror("Forking Program 1");
    }
    else if (current_pid==0){
        if(execl("./p1", input_file_name, pipe_1_write_str, sem_1_key_str, NULL)==-1){
            perror("Forking program");
        }
    }
    else{
        p1_pid = current_pid;
    }

    //fork program2 and pass args
    current_pid =fork();
    if (current_pid == -1){
        perror("Error forking program 2");
    }
    else if (current_pid == 0){
        if(execl("./p2", pipe_1_read_str, pipe_2_write_str, 
        sem_1_key_str, sem_2_key_str, shm_key_str, NULL)==-1){
            perror("Error forking program 2");
            exit(1);
        }
    }
    else{
        p2_pid = current_pid;
    }

    //fork program 3 and pass args
    current_pid = fork();
    if (current_pid == -1){
        perror("Error forking program 3");
    }
    else if (current_pid == 0){
        if(execl("./p3", output_file_name, pipe_2_read_str, sem_2_key_str,
        shm_key_str, NULL)==-1){
            perror("Error forking program 3");
            exit(1);
        }
    }
    else{
        p3_pid = current_pid;
    }

    waitpid(p1_pid, NULL, 0);
    close(pipe1fd[0]);
    close(pipe1fd[1]);
    waitpid(p2_pid, NULL, 0);
    close(pipe2fd[0]);
    close(pipe2fd[1]);
    waitpid(p3_pid, NULL, 0);

    //delete shared mem
    if (shmctl(shm_id, IPC_RMID, NULL)==-1){
        perror("Error deleting shm segment");
    }

    return 0;
}
