//Austin Holtz

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define PIPE_EMPTY 0
#define PIPE_FULL 1

/*
accept cli inputs
read from pipe1 and convert each word to pig latin
write converted word to second pipe
keep track of num words of each type and write them to shared memory
only 1 word in each pipe at a time (semaphores)
close pipe and terminate
*/

int type_one, type_two;

int isPunct (char c){
    int charAsInt = (int) c;


    if(charAsInt>=48 && charAsInt <= 57){
        return 0;
    }
    else if(charAsInt>=65 && charAsInt<=90){
        return 0;
    }
    else if(charAsInt>=97 && charAsInt<=122){
        return 0;
    }
    return 1;
}

int determineType(char* inString){
    char vowels[] = {'A', 'E', 'I', 'O', 'U', 'a', 'e', 'i', 'o', 'u'};
    char firstChar = inString[0];
    
    for(size_t i = 0; i < 10; i++)
    {
        if (firstChar == vowels[i]) return 1;
    }
    return 2;
}

char* convertToPigLat(char *inString){
    char *out = calloc(strlen(inString), sizeof(char));

    char charForEnding;
    switch (determineType(inString))
    {
        case 1:
            charForEnding = 'r';
            break;

        case 2:
            charForEnding = inString[0];
            break;
    }
    
    char lastChar = inString[strlen(inString)-1];
    char ending[] = {charForEnding, 'a', 'y', '\0', '\0'};
    if (isPunct(lastChar)){
        ending[3] = lastChar;
    }

    if (determineType(inString) == 1){
        type_one++;
        strncpy(out, inString, strlen(inString)-isPunct(lastChar));
        strcat(out, ending);
    }
    else {
        type_two++;
        strncpy(out, inString+1, strlen(inString)-1-isPunct(lastChar));
        strcat(out, ending);
    }
    return out;
}

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

    //gather cli inputs
    int pipe_1_read =  atoi(argv[0]);
    int pipe_2_write =  atoi(argv[1]);
    key_t sem_1_key = atoi(argv[2]);
    key_t sem_2_key = atoi(argv[3]);
    key_t shm_key = atoi(argv[4]);

    //get semaphores
    int sem_1_id = semget(sem_1_key, 0, 0);
    if (sem_1_id == -1){
        perror("Program 2: error getting semaphore 1 id");
        exit(1);
    }

    int sem_2_id = semget(sem_2_key, 0, 0);
    if (sem_2_id == -1){
        perror("Program 2: error getting semaphore 2 id");
        exit(1);
    }


    //get data from pipe
    int sem_1_value;
    int sem_2_val;

    char* input_buffer = calloc(50, sizeof(char));
    while(1){
        //read pipe 1
        while(semctl(sem_1_id, 0, GETVAL)==PIPE_EMPTY); //wait
        read(pipe_1_read, input_buffer, 50);
        if (strlen(input_buffer)==0) break;

        //convert to pig lat
        char* out = convertToPigLat(input_buffer);

        //write pipe 2
        while(semctl(sem_2_id, 0, GETVAL)==PIPE_FULL); //wait
        if(write(pipe_2_write, out, strlen(out)+1)==-1)perror("p2 writing to pipe 2");
        // printf("p2: %s\n", input_buffer);
        sem_ctl_union.val=PIPE_FULL;
        semctl(sem_2_id, 0, SETVAL, sem_ctl_union);
        sem_ctl_union.val=PIPE_EMPTY;
        semctl(sem_1_id, 0, SETVAL, sem_ctl_union);
    }
    while(semctl(sem_2_id, 0, GETVAL)==PIPE_FULL);
    write(pipe_2_write, "\0", 1);
    sem_ctl_union.val=PIPE_FULL;
    semctl(sem_2_id, 0, SETVAL, sem_ctl_union);

    //write to shared memory
    int shm_id = shmget(shm_key, 0, 0);
    int *shm_ptr = shmat(shm_id, NULL, 0);
    shm_ptr[0] = type_one;
    shm_ptr[1] = type_two;

    close(pipe_1_read);
    close(pipe_2_write);
    semctl(sem_1_id, 0, IPC_RMID);

    return 0;
}
