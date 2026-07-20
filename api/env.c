#include "env.h"
#include "config_file.h"
#include "utils.h"


int loadEnv(){
    FILE *env_file;
    char* line = NULL;
    size_t line_size = 0;
    char* path_env = NULL;

    if(!(path_env = compute_absolute_path(".env",FOLDER_CONFIG))){
        return -1;
    }
    env_file = fopen(path_env,"r");

    if(env_file == NULL){
        free(path_env);
        return -1;
    }
    while(getline(&line,&line_size,env_file) != -1){

        line[strcspn(line,"\r\n")] = '\0';
        char* ptr_to_delimiter = strchr(line,'=');
        if(ptr_to_delimiter){
            *ptr_to_delimiter = '\0';
            if(setenv(line,ptr_to_delimiter+1,1) != 0){
                fprintf(stderr,"[!] Failed to set env value\n");
                free(path_env);
                return -1;
            }
        }

    }
    free(line);
    free(path_env);
    fclose(env_file);

    return 0;

}