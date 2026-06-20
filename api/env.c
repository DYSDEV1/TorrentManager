#include "env.h"


int loadEnv(){
    FILE *env_file;
    char* line = NULL;
    size_t line_size = 0;
    env_file = fopen(".env","r");
    if(env_file == NULL){
        fprintf(stderr,"[!] Failed to load file env\n");
        return -1;
    }
    while(getline(&line,&line_size,env_file) != -1){

        line[strcspn(line,"\r\n")] = '\0';
        char* ptr_to_delimiter = strchr(line,'=');
        if(ptr_to_delimiter){
            *ptr_to_delimiter = '\0';
            if(setenv(line,ptr_to_delimiter+1,1) != 0){
                fprintf(stderr,"[!] Failed to set env value\n");
                return -1;
            }
        }

    }
    free(line);

    fclose(env_file);

    return 0;

}