#include <stdio.h>
#include "./include/mfs.h"
#include <stdlib.h>

int main() {
    // open_file_system("./file") ;
    create_file_system("./test.img") ;
    // int status = create_file_system("/home/ayushi/Documents/OS/gemOS/gemdisk.img") ;
    // printf("%d\n",status) ;
    // ls("");

    // status_superblock() ;
    create_file_or_dir("","tinglatxt",0) ;
    create_file_or_dir("","ayushi",0) ;
    create_file_or_dir("","tt",0) ;
    ls("") ;

    mfs_write("tt","myName",16) ;
    mfs_write("tinglatxt","myName2",17) ;
    mfs_write("ayushi","myName3",17) ;

    read_file("tt") ;
    read_file("tinglatxt") ;
    read_file("ayushi") ;

    create_file_or_dir("","desktop",1) ;
    create_file_or_dir("","documents",1) ;
    create_file_or_dir("","downloads",1) ;
    ls("") ; 

    create_file_or_dir("desktop","mycomputer",0) ;
    ls("desktop") ;
    delete_file("tt") ;
    ls("") ;
    // read_file("tt") ;

    mfs_write("ayushi","to use the unsetted data block",31) ;
    read_file("ayushi") ;
    // read_file("tt") ;
    print_inode(7) ;
    check_inode_bit(7);
    delete_directory("desktop") ;
    print_inode(7) ;
    check_inode_bit(7) ;
    ls("") ;

    close_filesystem() ;
    return 0 ;
}