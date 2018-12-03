#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

size_t getFileSize(char *fileName){
    FILE *fp = fopen(fileName, "r");
    size_t out = 0;
    while(fgetc(fp) != EOF){
        out++;
    }
    fclose(fp);
    return out;
}

char *readFile(char *fileName){
    size_t fileSize = getFileSize(fileName);
    int fileID = open(fileName, O_RDONLY);
    void *out = calloc(fileSize+1, sizeof(char));
    read(fileID, out, fileSize);
    close(fileID);
    return (char*) out;
}

char **splitString(char *string, char delimiter){
    int stringLength = strlen(string);
    int wordCount = 0;
    
    //count words and change spaces to null terminators
    for(size_t i = 0; i < stringLength; i++){
        if (string[i]==delimiter){
            if (string[i-1]!=delimiter){
                wordCount++;
            }
            string[i] = '\0';
        }
    }
    wordCount++;

    //create array of char**
    char **out = calloc(wordCount+1, sizeof(char**));

    //fill out with pointers to each word
    int currentOutIndex = 0, currentWordIndex = 0;
    
    for(size_t i = 0; i < stringLength; i++)
    {
        if (string[i] == '\0') {
            if (string[i-1] != '\0') { //check for multiple spaces
                out[currentOutIndex++] = &string[currentWordIndex];
            }
            currentWordIndex = i+1;
        }
    }
 
    out[currentOutIndex++] = &string[currentWordIndex];

    return out;
}