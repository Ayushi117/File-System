all: mkfs ls create read_file write_file check_inode_bit print_inode inode_of delete_file delete_directory

files = mfs.c include/mfs.h
CFLAGS  = -I./include -g

mkfs: mkfs.c $(files)
	gcc $(CFLAGS) mkfs.c mfs.c -o mkfs
ls: ls.c $(files)
	gcc $(CFLAGS) mfs.c ls.c -o ls
create: create.c $(files)
	gcc $(CFLAGS) mfs.c create.c -o create
read_file: read_file.c $(files)
	gcc $(CFLAGS) read_file.c mfs.c -o read_file
write_file: write_file.c $(files)
	gcc $(CFLAGS) write_file.c mfs.c -o write_file
check_inode_bit: check_inode_bit.c $(files)
	gcc $(CFLAGS) check_inode_bit.c mfs.c -o check_inode_bit
print_inode: print_inode.c $(files)
	gcc $(CFLAGS) print_inode.c mfs.c -o print_inode
inode_of: inode_of.c $(files)
	gcc $(CFLAGS) inode_of.c mfs.c -o inode_of
delete_file: delete_file.c $(files)
	gcc $(CFLAGS) delete_file.c mfs.c -o delete_file
delete_directory: delete_directory.c $(files)
	gcc $(CFLAGS) delete_directory.c mfs.c -o delete_directory
clean:
	rm -f mkfs ls create read_file write_file check_inode_bit print_inode inode_of delete_file

# SRCS = mfs.c mkfs.c ls.c
# OBJS = mkfs.o ls.o

# CFLAGS  = -I../include

# all: $(OBJS) 

# %o: %c
# 	gcc -c $(CFLAGS) $< -o $@ 

# .Phony: clean
# clean:
# 	rm -f *.o;