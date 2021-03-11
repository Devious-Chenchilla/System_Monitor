#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h> 
#include <string.h>
#include <ctype.h>

#include "struct_process.h"
#include "functions.h"
#include "functions.c"
#include "system.h"

#define PROC_DEVICES_DIR "/proc/"
static const int proc_devices_dir_len = sizeof(PROC_DEVICES_DIR) - 1;

//struct process_t procs;

#define SIZE 50
    struct process_t process;


void main(int argc, char **argv) {


    
}
