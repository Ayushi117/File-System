#ifndef __FS_H_
#define __FS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef unsigned int u32;
typedef          int s32;
typedef unsigned short u16;
typedef int      short s16;
typedef unsigned char u8;
typedef char          s8;

#if __x84_64__
typedef unsigned long u64;
typedef long s64
#else
typedef unsigned long long u64;
typedef long long s64;
#endif


#define AVG_FILE_SIZE 8  // avg. file size in blocks, used to calc #of files
#define BLOCK_SIZE 0x1000  //4kiB
// #define DISK_SIZE 0x10000  //4GiB
// #define NUM_FILES 4096
#define MAX_FILE_NAME_LEN 124
#define MAX_INODES 4294967295 //UINT_MAX
#define MAX_BLOCKS 4294967295 //UINT_MAX

/*
block offset = 0 
size of SB = 1 block
*/
struct super_block {
	u16 block_size;		//size of 1 block - 4kiB
	u32 total_block_count;
	u32 inode_count;	//
	u32 inode_bitmap_offset;
	u32 data_block_bitmap_offset;
	u32 inode_table_offset;   // starting of the inode table
	u32 data_block_offset;    // starting of the data blocks
	u32 data_block_count;
	u32 free_inode_count;
	u32 free_data_block_count;
	u32 root_directory_inode;
	u16 inode_size;
	u32 wtime;	
	u32 number_of_inodes_in_1_block;
};

/*
size of inode = 64 byte
*/
struct inode {
	u8 mode[8];
	u32 size;
	u32 parent;
	u32 block[12];   // 10 direct, one indirect and one double indirect
};

struct directory_entry {
	u32 inode;
	char name[MAX_FILE_NAME_LEN];
};

extern int create_file_system( const char* path);
extern int open_file_system(const char* path);
extern u32 alloc_block() ;
extern void set_inode(u16 inode_idx) ;
extern void unset_inode(u16 inode_idx) ;
void set_data_block_bit(u16 idx) ;
void unset_data_block_bit(u16 idx) ;
void mfs_write(const char* filename, char* buffer, u32 data_len) ;
u32 create_file_or_dir(const char* file_path, const char* filename, u8 mode) ;
void append_directory_entry_in_directory(struct inode* dir_inode, struct directory_entry* new_dir_entry) ;
void write_inode(struct inode inode, u32 ino) ;
u32 find_empty_inode() ;
u32 find_directory_or_file(const char* dir_path, u32 cur_dir_inode) ;
u32 find_entry_in_directory(const char* name, u32 dir_inode) ;
struct inode read_inode(u32 ino) ;
void ls(const char* dir_path) ;
void read_block(u32 offset,char* ptr) ;
void write_block(u32 offset,char* ptr) ;
void read_file(const char* filename) ;
u32 get_next_file_block_idx(struct inode in, u32 block_number) ;
void delete_file(const char* filename) ;
void clear_data_bits(struct inode in) ;
void remove_entry_from_directory(u32 ino) ;
void move_last_dir_entry(u32 last_offset, u32 dir_entry_offset) ;
u32 get_dir_entry_offset(struct inode dir, u32 file_ino) ;
void delete_directory(const char* dirname) ;
void delete_directory_files(struct inode dir, const char* dirname) ;
// void clear_directory_data_bits(struct inode dir) ;
void close_filesystem() ;
u8 check_inode_bit(u32 ino) ;	
u8 check_data_bit(u32 data_bit) ;
void status_superblock() ;
void print_inode(u16 ino) ;
void print_dir_entry(u32 offset) ;


#endif
