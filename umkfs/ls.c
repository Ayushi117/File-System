#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<fcntl.h>
#include <mfs.h>

void usage_error(char *cmd)
{
  fprintf(stderr, "Invalid arguments. Usage: %s {path to the raw disk image file} {path to the directory in which ls need to be performed}\n", cmd);
  exit(-1);   
}

int main(int argc, char **argv)
{
  if(argc != 3){
    usage_error(argv[0]);
  }	  
  
  if(access(argv[1],O_RDWR)){
    usage_error(argv[0]);
  }

  assert(open_file_system(argv[1])==1) ;
  ls(argv[2]) ;
  close_filesystem() ;

  return 0;
}
