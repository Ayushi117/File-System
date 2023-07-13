#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<fcntl.h>
#include <mfs.h>

void usage_error(char * cmd) {
    fprintf(stderr, "Invalid arguments\nUsage: %s {path to the raw disk image file}\n", cmd);
}

int main(int argc, char** argv) {
    if(argc != 5) {
        usage_error(argv[0]) ;
        exit(-1) ;
    }

    if(access(argv[1],O_RDWR)) {
        usage_error(argv[0]) ;
        exit(-1) ;
    }

    assert(open_file_system(argv[1])==1) ;
    mfs_write(argv[2],argv[3],atoi(argv[4])) ;
    close_filesystem() ;
    
    return 0 ;
}