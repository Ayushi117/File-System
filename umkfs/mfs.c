#include <mfs.h>

struct super_block* super_block; 
FILE *disk;

/*
 * This creates a file system on raw file specified in the 'path argument'
 * Return: 1 in successful creation and -1 if unsuccessful 
 *
 * Notes: root is ""
 * TODO: usage of wtime 
 */
int create_file_system( const char* path) {	
	disk = fopen(path,"r+");
	if(disk==NULL) {
		printf("File not found on this path - %s\n",path) ;
		return -1 ;
	}
	
	fseek(disk,0L,SEEK_END);
	u64 disk_size = ftell(disk);
	if(disk_size/BLOCK_SIZE>MAX_BLOCKS) {
		printf("File size too big, can't be supported\n");
		return -1 ;
	}

	super_block = (struct super_block*)malloc(sizeof(struct super_block));
	
	super_block->block_size = BLOCK_SIZE;
	super_block->total_block_count = disk_size/BLOCK_SIZE;
	super_block->inode_count = super_block->total_block_count/AVG_FILE_SIZE;
	super_block->inode_bitmap_offset = BLOCK_SIZE ; // 0 for SB and inode bitmap comes just after that
	super_block->data_block_bitmap_offset = super_block->inode_bitmap_offset + (((super_block->inode_count)/8+BLOCK_SIZE-1)/BLOCK_SIZE)*BLOCK_SIZE;
	super_block->data_block_count =  disk_size/BLOCK_SIZE;
	super_block->inode_table_offset = super_block->data_block_bitmap_offset + (((super_block->data_block_count)/8+BLOCK_SIZE-1)/BLOCK_SIZE)*BLOCK_SIZE;
	super_block->free_inode_count = super_block->inode_count; 
	super_block->data_block_offset = super_block->inode_table_offset + (((super_block->inode_count)*sizeof(struct inode)+BLOCK_SIZE-1)/BLOCK_SIZE)*BLOCK_SIZE ;
	super_block->free_data_block_count = super_block->data_block_count;
	super_block->root_directory_inode = 0; 
	super_block->inode_size = sizeof(struct inode);
	// super_block->wtime = 
	super_block->number_of_inodes_in_1_block = BLOCK_SIZE/sizeof(struct inode); 

	fseek(disk,0L,SEEK_SET);
	fwrite((void *)super_block,sizeof(struct super_block),1,disk);  //writing super_block on disk

	char block[BLOCK_SIZE];
	memset(block,0,sizeof(block));
	for(int i=super_block->inode_bitmap_offset; i<super_block->inode_table_offset; i+=BLOCK_SIZE) {
		fseek(disk, i, SEEK_SET);
		fwrite(block, BLOCK_SIZE, 1, disk);
	}

	// creating root directory
	set_inode(0);
	struct inode r ;
	r.size=0 ;
	r.mode[0]=1; //1==directory, 0==file
	r.parent=-1;

	fseek(disk,super_block->inode_table_offset,SEEK_SET);
	fwrite((void *)&r,sizeof(struct inode),1,disk);

	// struct directory_entry dir;
	// dir.inode=0;
	// char name[] = "" ;
	// strcpy(dir.name, name);

	// fseek(disk,super_block->data_block_offset+r.block[0],SEEK_SET) ;
	// fwrite((void *)&dir,sizeof(struct directory_entry),1,disk);	

	// fclose(disk);

	return 1;
}

/*
* This opens the previously created file system
* sets up the disk and super_block pointers
*/
int open_file_system(const char* path) {
	disk = fopen(path,"r+");
	if(disk==NULL) return 0 ;
	fseek(disk,0L,SEEK_SET) ;
	super_block = (struct super_block*)malloc(sizeof(struct super_block));
	fread((void*)super_block,sizeof(struct super_block),1,disk) ;
	return 1 ;
}

/*
* This allocates the first unused data block for use
* Returns : the data block index of the allocated block
*/
u32 alloc_block() {
	if(super_block->free_data_block_count==0) { printf("Error\n"); return -1 ; }

	fseek(disk,super_block->data_block_bitmap_offset,SEEK_SET);
	u32 i,j;
	char data_bitmap_subset[BLOCK_SIZE];
	for( i=0; i<(super_block->data_block_count)/BLOCK_SIZE; i++) {
		fread((void *)&data_bitmap_subset,BLOCK_SIZE,1,disk);
		for(j=0;j<BLOCK_SIZE;j++) 
			if(data_bitmap_subset[j]!=255) {
				u8 rev = data_bitmap_subset[j];
				rev = ~rev ;
				u8 k=0 ;
				while(rev) { rev/=2 ; k++ ; }
				k=8-k ;
				u32 t=i*BLOCK_SIZE*8+j*8+k;
				set_data_block_bit(t);
				return t;
			}
	}
	printf("Error\n"); 
	return -1 ; 
}

/*
* This sets the inode bit of the given inode to 1 (i.e. in use)
* Note: implementation of this is done via bit manipulation
* and the inode bitmap data is read in such a way that 1 byte of data is 
* read in which the inode index falls
*/
void set_inode(u16 inode_idx) {
	u8 new = 0;
	u8 old ;
	new |= 1<<(7-(inode_idx%8)) ;
	fseek(disk,(super_block->inode_bitmap_offset+inode_idx/8),SEEK_SET);
	fread((void*)(&old),1,1,disk);
	new |= old ;
	fseek(disk,(super_block->inode_bitmap_offset+inode_idx/8),SEEK_SET);
	fwrite((void*)(&new),1,1,disk);
}

/*
* This unsets the inode bit of the given inode to 0 (i.e. not in use)
*/
void unset_inode(u16 inode_idx) {
	u8 new = 255, old ;
	new ^= 1<<(7-(inode_idx%8)) ;
	fseek(disk,(super_block->inode_bitmap_offset+inode_idx/8),SEEK_SET);
	fread((void*)(&old),1,1,disk);
	new &= old ;
	fseek(disk,(super_block->inode_bitmap_offset+inode_idx/8),SEEK_SET);
	fwrite((void*)(&new),1,1,disk);
}

/*
*  This sets the data block bit of the given data block index to 1 (i.e. in use)
*/
void set_data_block_bit(u16 idx) {
	u8 new = 0, old ;
	new |= 1<<(7-(idx%8)) ;
	fseek(disk,(super_block->data_block_bitmap_offset+idx/8),SEEK_SET);
	fread((void*)(&old),1,1,disk);
	new |= old ;
	fseek(disk,(super_block->data_block_bitmap_offset+idx/8),SEEK_SET);
	fwrite((void*)(&new),1,1,disk);
}

/*
*  This unsets the data block bit of the given data block index to 0 (i.e. not in use)
*/
void unset_data_block_bit(u16 idx) {
	u8 new = 255, old ;
	new ^= 1<<(7-(idx%8)) ;
	fseek(disk,(super_block->data_block_bitmap_offset+idx/8),SEEK_SET);
	fread((void*)(&old),1,1,disk);
	new &= old ;
	fseek(disk,(super_block->data_block_bitmap_offset+idx/8),SEEK_SET);
	fwrite((void*)(&new),1,1,disk);
}

/*
* This writes the content of a file in the file given in filename 
* and writes data_len amount of data from buffer
* Note: filename should be complete address of the file from root
* root is - ""
*/
void mfs_write(const char* filename, char* buffer, u32 data_len) {
	u32 file_inode = find_directory_or_file(filename,0) ;
	if(file_inode==-1) {
		char subpath[MAX_FILE_NAME_LEN], file[MAX_FILE_NAME_LEN] ;
		u32 s=strlen(filename),j=0, i=s-1 ;
		while(i>=0 && filename[i]!='/') i-- ;
		while(j<i) {
			subpath[j]=filename[j] ;
			j++;
		}
		subpath[j]='\0' ;
		j=0;
		i++ ;
		while(i<s) {
			file[j]=filename[i] ;
			j++ ; i++ ;
		}
		file[j]='\0';
		file_inode = create_file_or_dir(subpath,file,0) ; 
	}
	u32 i=0 ,j=0;
	struct inode in = read_inode(file_inode) ;
	clear_data_bits(in) ;
	in.size=0 ;
	while(i<data_len) {
		if(j<10) {
			in.block[j] = alloc_block() ;
			u32 offset = super_block->data_block_offset + BLOCK_SIZE*in.block[j] ;
			fseek(disk,offset,SEEK_SET) ;
			fwrite((void *)buffer,BLOCK_SIZE,1,disk) ;
			j++ ;
			if(data_len-i<BLOCK_SIZE) in.size += data_len-i ;
			else in.size += BLOCK_SIZE ;
			// in.size += min(BLOCK_SIZE,data_len-i) ;
			i+=BLOCK_SIZE ;
		}
		else if(j<10 + BLOCK_SIZE/sizeof(u32)) {
			char indirect_pointer_buffer[BLOCK_SIZE] ;
			if(in.size==BLOCK_SIZE*10) in.block[10] = alloc_block() ;
			u32 indirect_pointer_offset = super_block->data_block_offset + BLOCK_SIZE*in.block[10] ;
			// fseek(disk,indirect_pointer_offset,SEEK_SET) ;
			read_block(indirect_pointer_offset,indirect_pointer_buffer); 
			u32* offset_ptr=(u32*)indirect_pointer_buffer; 
			for(int i=0 ; i<BLOCK_SIZE/sizeof(u32); i++) {
				u32 offset = alloc_block() ;
				*offset_ptr = offset ;
				offset_ptr++ ;
				// u32 offset = super_block->data_block_offset + BLOCK_SIZE*(*offset_ptr) ;
				fseek(disk,offset,SEEK_SET) ;
				fwrite((void *)buffer,BLOCK_SIZE,1,disk) ;	
				j++ ;
				if(data_len-i<BLOCK_SIZE) in.size += data_len-i ;
				else in.size += BLOCK_SIZE ;
				i+=BLOCK_SIZE ;
				if(i>data_len) break;
			}
			write_block(indirect_pointer_offset,indirect_pointer_buffer) ;
		}
		else if(j<(10 + BLOCK_SIZE/sizeof(u32) + (BLOCK_SIZE/sizeof(u32))*(BLOCK_SIZE/sizeof(u32)))){
			char double_indirect_pointer[BLOCK_SIZE] ;
			if(in.size==BLOCK_SIZE*(10+BLOCK_SIZE/sizeof(u32))) in.block[11] = alloc_block() ;
			u32 double_indirect_pointer_offset = super_block->data_block_offset + BLOCK_SIZE*in.block[11] ;
			read_block(double_indirect_pointer_offset,double_indirect_pointer) ;
			u32* double_indirect_pointer_ptr = (u32*)double_indirect_pointer ;
			
			for(int k=0 ; k<BLOCK_SIZE/sizeof(u32) && i<data_len ; k++) {
				char indirect_pointer_buffer[BLOCK_SIZE] ;
				u32* indirect_pointer_buffer_ptr = (u32*)indirect_pointer_buffer ;
				u32 offset_ptr= alloc_block() ;
				*double_indirect_pointer_ptr = offset_ptr; 
				double_indirect_pointer_ptr++ ;
				for(int i=0 ; i<BLOCK_SIZE/sizeof(u32); i++) {
					u32 offset = alloc_block() ;
					write_block(super_block->data_block_offset + BLOCK_SIZE*offset,buffer) ;
					*indirect_pointer_buffer_ptr = offset; 
					indirect_pointer_buffer_ptr++ ;
					j++ ;
					if(data_len-i<BLOCK_SIZE) in.size += data_len-i ;
					else in.size += BLOCK_SIZE ;
					i+=BLOCK_SIZE ;
					if(i>data_len) break ;
				}
				write_block(offset_ptr,indirect_pointer_buffer) ;
			}
			write_block(double_indirect_pointer_offset,double_indirect_pointer) ;
		}
		else {
			printf("file size too big\n") ;
			return ;
		}
		write_inode(in,file_inode) ;
	}
}

/* 
* This creates a file or a directory (mode=0 for file and =1 for directory)
* in a directory given by file_path
* Returns: the inode number of the newly created file or directory
* Note: file_path is the complete path of the directory 
*/
u32 create_file_or_dir(const char* file_path, const char* filename, u8 mode) {
	if(super_block->free_inode_count==0) return 0;

	u32 dir_ino = find_directory_or_file(file_path,0) ;
	struct inode dir_inode = read_inode(dir_ino);

	struct directory_entry new_dir_entry ;
	strcpy(new_dir_entry.name,filename); 
	new_dir_entry.inode = find_empty_inode();

	set_inode(new_dir_entry.inode) ;
	struct inode new_inode ;
	new_inode.mode[0]=mode ; //0 for file, 1 for directory
	new_inode.size=0 ;
	new_inode.parent=dir_ino ;
	write_inode(new_inode,new_dir_entry.inode) ;

	append_directory_entry_in_directory(&dir_inode,&new_dir_entry) ;
	write_inode(dir_inode,dir_ino) ;
	
	return new_dir_entry.inode ;
}

/*
* This adds a directory entry to a given directory
* TODO: double indirect pointer implementation
*/
void append_directory_entry_in_directory(struct inode* dir_inode, struct directory_entry* new_dir_entry) {
	u32 used_size = dir_inode->size;
	u32 block_to_use = used_size/BLOCK_SIZE;
	u32 block_offset = used_size%BLOCK_SIZE;
	u32 dir_entries_per_block = BLOCK_SIZE/sizeof(struct directory_entry) ;
	u32 new_block_added = 0; 
	if(block_offset==0 && block_to_use==0) {
		new_block_added=1 ;
	}
	if(block_offset==0 && block_to_use!=0) { 
		block_to_use+=1 ; 
		new_block_added = 1 ;
		dir_inode->size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ; 
	}
	else if(block_offset+sizeof(struct directory_entry)>BLOCK_SIZE) { 
		block_to_use+=1 ; 
		new_block_added=1 ;
		block_offset=0;
		dir_inode->size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ;  
	}
	dir_inode->size += sizeof(struct directory_entry) ;

	u32 blocks_in_indirect_pointer = BLOCK_SIZE/sizeof(u32) ;
	u32 data_block_index ;

	if(block_to_use<10) {
		if(new_block_added) {
			dir_inode->block[block_to_use] = alloc_block() ;
		}
		data_block_index=dir_inode->block[block_to_use] ;
	}
	else if(block_to_use<10+blocks_in_indirect_pointer) {
		block_to_use -=10 ;
		u32 indirect_pointer_block_offset = super_block->data_block_offset + dir_inode->block[10]*BLOCK_SIZE ;
		char buffer[BLOCK_SIZE] ;
		read_block(indirect_pointer_block_offset ,buffer) ;
		u32* indirect_data_blocks = (u32*) buffer;
		if(new_block_added) {
			indirect_data_blocks[block_to_use] = alloc_block();
			write_block(indirect_pointer_block_offset, buffer);
		}
		data_block_index = indirect_data_blocks[block_to_use];
	}
	else {
		// double indirect not implemented rn
	}


	u32 write_offset = super_block->data_block_offset + data_block_index*BLOCK_SIZE + block_offset ;
	fseek(disk,write_offset,SEEK_SET) ;
	fwrite((void *)new_dir_entry,sizeof(struct directory_entry),1,disk) ;
}

/*
* This writes/overwrites an existing inode in the inode table
*/
void write_inode(struct inode inode, u32 ino) {
	u32 inode_block_number = ino/super_block->number_of_inodes_in_1_block ;
	u32 inode_offset = (ino%super_block->number_of_inodes_in_1_block)*sizeof(struct inode) ;
	u32 offset = super_block->inode_table_offset + inode_block_number*BLOCK_SIZE + inode_offset;
	fseek(disk,offset,SEEK_SET) ;
	fwrite((void *)&inode,sizeof(struct inode),1,disk);
}

/*
* This finds the first unused inode from the inode bitmap
* Returns: the inode index of the first unused inode
*/
u32 find_empty_inode() {
	if(super_block->free_inode_count==0) return -1 ;

	fseek(disk,super_block->inode_bitmap_offset,SEEK_SET) ;
	char buffer[BLOCK_SIZE] ;
	u32 offset = super_block->inode_bitmap_offset, block_read=0 ;
	while(offset!=super_block->data_block_bitmap_offset) {
		read_block(offset,buffer) ;
		for(int i=0 ; i<BLOCK_SIZE ; i++) {
			u8 byte = buffer[i] ;
			if(byte!=255) {
				u8 k=0 ;
				byte= ~byte ;
				while(byte) { k++ ; byte/=2 ; }
				k=8-k ;
				u32 empty_inode_idx = block_read*BLOCK_SIZE*8 + i*8 + k ;
				return empty_inode_idx ;
			}
		}
		block_read++ ;
		offset+=BLOCK_SIZE ;
	}

	return -1;

// //////////
	// fseek(disk,super_block->inode_bitmap_offset,SEEK_SET) ;
	// u32 used_inode = super_block->inode_count - super_block->free_inode_count ;
	// char buffer[BLOCK_SIZE] ;
	// u32 cur_block=0 ;
	// u8* ptr ;
	// while(used_inode!=0) {
	// 	read_block(0,buffer) ;
	// 	ptr = (u8*)buffer ;
	// 	u32 i=0 ;
	// 	while(used_inode!=0 && i<BLOCK_SIZE) {
	// 		u8 val = *ptr ;
	// 		if(val==255) {
	// 			used_inode-=8 ;
	// 			ptr++; 
	// 			i++;
	// 			continue ;
	// 		}
	// 		u8 num = (~(*ptr))/2 ,msb=0 ;
	// 		while(num) {
	// 			num/=2 ;
	// 			msb++ ;
	// 		}
	// 		u8 bit_offset = 8-msb ;
	// 		u32 inode_offset = cur_block*BLOCK_SIZE*8 + 8*i + bit_offset ;
	// 		return inode_offset ;
	// 	}
	// 	cur_block+=1 ;
	// }
	// return cur_block*BLOCK_SIZE ;
}

/*
* This finds a directory or file in a particular directory denoted by its inode
* dir_path is th path of the file or directory from cur_dir
* Return: the inode number of the found file or directory, -1/0 if not found
* TODO: implementing '.' and '..' in path of a file or directory 
*/
u32 find_directory_or_file(const char* dir_path, u32 cur_dir_inode) {
	if(strlen(dir_path)==0) return cur_dir_inode ;
	
	char dir[MAX_FILE_NAME_LEN] , subpath[MAX_FILE_NAME_LEN] ;
	u32 i=0, s=strlen(dir_path);
	while(i<s && dir_path[i]!='/') i++;
	strncpy(dir, dir_path, i);
	dir[i]='\0' ;
	if(dir_path[i]=='/') i++ ;
	strcpy(subpath, dir_path+i);

	u32 next=find_entry_in_directory(dir,cur_dir_inode);

	//TODO
	// if(strcmp(dir,".")==0) return find_directory_or_file(subpath,next);
	// if(strcmp(dir,"..")==0) {
	// 	struct inode in = read_inode(cur_dir_inode);
	// 	if(in.parent==-1) { printf("error\n"); return -1 ; }
	// 	return find_directory_or_file(subpath,in.parent);
	// }
	if(next == cur_dir_inode){
		return cur_dir_inode;
	}
	return find_directory_or_file(subpath,next);
}

/*
* This is helper function of find_directory_or_file
* This finds the directory or file in directpry denoted by dir_inode
* Return: the inode number of found file/directory 
* Note: name is just the name of file/directory to search (not path)
* TODO: implementation of double indirect pointer
*/
u32 find_entry_in_directory(const char* name, u32 dir_inode) {
	u32 read_size=0 ;
	char buffer[BLOCK_SIZE] ;
	struct inode dir = read_inode(dir_inode) ;
	if(dir.mode[0] == 0) {
		printf("Error: %s Not a directory\n", name);
		return 0;
	}
	u32 dir_entries_per_block = BLOCK_SIZE/sizeof(struct directory_entry) ;
	for(int i=0 ; i<10 && read_size<dir.size ; i++) {
		u32 offset = super_block->data_block_offset + BLOCK_SIZE*dir.block[i] ;
		read_block(offset,buffer) ;
		struct directory_entry* ptr=(struct directory_entry*)buffer ;
		for(int j=0 ; j<dir_entries_per_block ; j++) {
			if(strcmp(name,ptr->name)==0) return ptr->inode ;
			ptr++ ;
			read_size += sizeof(struct directory_entry) ;
			if(read_size>=dir.size) break ;
		}
		read_size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ;
	}

	char indirect_pointer_buffer[BLOCK_SIZE] ;
	u32 indirect_pointer_offset = super_block->data_block_offset + BLOCK_SIZE*dir.block[10] ;
	fseek(disk,indirect_pointer_offset,SEEK_SET) ;
	fread((void *)indirect_pointer_buffer,BLOCK_SIZE,1,disk) ;
	u32* offset_ptr=(u32*)indirect_pointer_buffer; 
	for(int i=0 ; i<BLOCK_SIZE/sizeof(u32) && read_size<dir.size; i++) {
		u32 offset = super_block->data_block_offset + BLOCK_SIZE*(*offset_ptr) ;
		read_block(offset,buffer) ;
		struct directory_entry* ptr=(struct directory_entry*) buffer ;
		for(int j=0 ; j<dir_entries_per_block && read_size<dir.size; j++) {
			if(strcmp(name,ptr->name)==0) return ptr->inode ;
			ptr++ ;
			read_size += sizeof(struct directory_entry) ;
		}
		read_size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ;
		offset_ptr++ ;
	}

	//double indirect pointer not implemented rn

	printf("file/dir not found error\n") ;
	return 0 ;
}

/*
* This reads an inode from the disk
* Return: the inode in struct inode form
*/
struct inode read_inode(u32 ino) {
	u32 inode_block_number = ino/super_block->number_of_inodes_in_1_block ;
	u32 inode_offset = (ino%super_block->number_of_inodes_in_1_block)*sizeof(struct inode) ;
	u32 offset = super_block->inode_table_offset + inode_block_number*BLOCK_SIZE + inode_offset;
	fseek(disk,offset,SEEK_SET) ;
	struct inode in;
	fread((void *)&in,sizeof(struct inode),1,disk) ;
	return in ;
}

/*
* This lists all the components of a directory
* Note: dir_path is the complete path
*/
void ls(const char* dir_path) {
	u32 dir_inode = find_directory_or_file(dir_path,0) ;
	u32 read_size = 0 ; 
	char buffer[BLOCK_SIZE] ;
	struct inode dir = read_inode(dir_inode) ;
	u32 dir_entries_per_block = BLOCK_SIZE/sizeof(struct directory_entry) ;
	for(int i=0 ; i<10 && read_size<dir.size ; i++) {
		u32 offset = super_block->data_block_offset + BLOCK_SIZE*dir.block[i] ;
		read_block(offset,buffer) ;
		struct directory_entry* ptr=(struct directory_entry*)buffer ;
		for(int j=0 ; j<dir_entries_per_block ; j++) {
			printf("%s\n",ptr->name) ;
			ptr++ ;
			read_size += sizeof(struct directory_entry) ;
			if(read_size >= dir.size) return ;
		}
		read_size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ;
	}

	char indirect_pointer_buffer[BLOCK_SIZE] ;
	u32 indirect_pointer_offset = super_block->data_block_offset + BLOCK_SIZE*dir.block[10] ;
	fseek(disk,indirect_pointer_offset,SEEK_SET) ;
	fread((void *)indirect_pointer_buffer,BLOCK_SIZE,1,disk) ;
	u32* offset_ptr=(u32*)indirect_pointer_buffer; 
	for(int i=0 ; i<BLOCK_SIZE/sizeof(u32); i++) {
		u32 offset = super_block->data_block_offset + BLOCK_SIZE*(*offset_ptr) ;
		read_block(offset,buffer) ;
		struct directory_entry* ptr=(struct directory_entry*)buffer ;
		for(int j=0 ; j<dir_entries_per_block && read_size<dir.size ; j++) {
			printf("%s\n",ptr->name) ;
 			ptr++ ;
			if(read_size >= dir.size) return ;
		}
		read_size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ;
		offset_ptr++ ;
	}

	//double indirect pointer not implemented rn

}

/*
* This reads one block of data from offset and is pointed by ptr
*/
void read_block(u32 offset,char* ptr) {
	fseek(disk,offset,SEEK_SET);
	fread(ptr,BLOCK_SIZE,1,disk);
}

/*
* This writes one block of data pointed by ptr at offset in disk
*/
void write_block(u32 offset,char* ptr) {
	fseek(disk,offset,SEEK_SET);
	fwrite((void *)ptr,BLOCK_SIZE,1,disk);
}

/*
* This reads and prints the content of a file denoted by filename
* Note: filename is complete path
*/
void read_file(const char* filename) {
	u32 ino = find_directory_or_file(filename,0) ;
	
	if(ino==-1) {
		printf("error file not found\n") ;
		return ;
	}

	struct inode inode = read_inode(ino) ;

	u32 cur_data_block=0, size_read=0, offset,data_block_idx ;
	char buffer[BLOCK_SIZE] ;
	char *ptr;

	while(size_read<inode.size) {
		data_block_idx = get_next_file_block_idx(inode,cur_data_block) ;
		offset = super_block->data_block_offset + BLOCK_SIZE*data_block_idx ;
		read_block(offset,buffer) ;
		ptr=buffer ;

		while(*ptr!='\0') {
			printf("%c",*ptr) ;
			ptr++ ;
		}
		printf("\n") ;

		cur_data_block++ ;
		size_read += BLOCK_SIZE ;	
	}
}

/*
* This is helper function of read_file
* This provides the offset of the next block to read for the file 
* given gown many blocks have already been read
* Return: offset of the data block to read next
*/
u32 get_next_file_block_idx(struct inode in, u32 block_number) {
	if(block_number*BLOCK_SIZE > in.size) return -1 ;
	u32 indirect_block_pointers = BLOCK_SIZE/sizeof(u32) ;
	if(block_number<10) {
		return in.block[block_number] ;
	}
	else if(block_number<10+indirect_block_pointers) {
		char buffer[BLOCK_SIZE] ;
		u32 offset = super_block->data_block_offset + in.block[10]*BLOCK_SIZE ;
		read_block(offset, buffer) ;
		u32* ptr = (u32*)buffer ;
		block_number -=10 ;
		ptr += block_number ;
		return *ptr ;
	}
	else {
		char pointer_buffer[BLOCK_SIZE] ;
		u32 pointer_offset = super_block->data_block_offset + in.block[11]*BLOCK_SIZE ;
		read_block(pointer_offset,pointer_buffer) ;
		u32 *pointer_ptr = (u32*)pointer_buffer ;
		block_number -= 10 + indirect_block_pointers ;
		u32 pointer_block_number = block_number/(BLOCK_SIZE/sizeof(u32)) ;
		u32 direct_pointer_block_offset = *(pointer_ptr + pointer_block_number) ; 
		char direct_pointer_block[BLOCK_SIZE] ;
		read_block(direct_pointer_block_offset,direct_pointer_block) ;
		u32 *ptr = (u32*)direct_pointer_block ;
		u32 offset = block_number%(BLOCK_SIZE/sizeof(u32)) ;
		return (*ptr+offset) ;
	}
}

/*
* This removes the file by unsetting all its data blocks and unsetting its inode
* Note: The content of the file is not actually deleted just the refernces are removed
*/
void delete_file(const char* filename) {
	u32 file_inode = find_directory_or_file(filename,0) ;
	struct inode in = read_inode(file_inode) ;
	remove_entry_from_directory(file_inode) ;
	clear_data_bits(in) ;	
	unset_inode(file_inode) ;
}

/*
* This removes an entry from directory denoted by inode number ino
* by replacing the last directory entry with the directory entry to be removed
*/
void remove_entry_from_directory(u32 ino) {
	struct inode in = read_inode(ino) ;
	struct inode parent_inode = read_inode(in.parent) ;
	u32 dir_entry_offset = get_dir_entry_offset(parent_inode,ino) ;
	if(dir_entry_offset==-1) { return ; }

	u32 remove_last_data_block=0 ;
	u32 entries_per_block = BLOCK_SIZE/sizeof(struct directory_entry) ;
	u32 last_offset ;
	if(parent_inode.size <=10*BLOCK_SIZE) {
		u32 i = parent_inode.size/BLOCK_SIZE , j = parent_inode.size - i*BLOCK_SIZE ;
		if(j==0) {
			i-- ;
			j += (entries_per_block-1)*sizeof(struct directory_entry) ;
		}
		else {
			j -= sizeof(struct directory_entry) ;
		}
		if(j==0) {
			remove_last_data_block=1 ;
			parent_inode.size -= BLOCK_SIZE - sizeof(struct directory_entry)*entries_per_block ;
		}
		last_offset = super_block->data_block_offset + parent_inode.block[i]*BLOCK_SIZE + j ; 
		move_last_dir_entry(last_offset,dir_entry_offset) ;
		if(remove_last_data_block) {
			unset_data_block_bit(parent_inode.block[i+1]) ;
		}
	}
	else if(parent_inode.size <= (10+BLOCK_SIZE/sizeof(u32))*BLOCK_SIZE) {
		char indirect_pointer[BLOCK_SIZE] ;
		u32 offset = super_block->data_block_offset + parent_inode.block[10]*BLOCK_SIZE ;
		read_block(offset,indirect_pointer) ;
		u32* indirect_pointer_ptr = (u32*)indirect_pointer ;
		u32 indirect_block_no = parent_inode.size/BLOCK_SIZE -10 ;
		u32 offset_in_indirect_block = parent_inode.size - (indirect_block_no+10)*BLOCK_SIZE ;

		if(offset_in_indirect_block==0) {
			indirect_block_no-- ;
			offset_in_indirect_block += (entries_per_block-1)*sizeof(struct directory_entry) ; 
		}
		else {
			offset_in_indirect_block -= sizeof(struct directory_entry) ;
		}

		if(offset_in_indirect_block==0) {
			remove_last_data_block=1 ;
			parent_inode.size -= BLOCK_SIZE - sizeof(struct directory_entry)*entries_per_block ;
		}

		if(indirect_block_no==-1) {
			last_offset = super_block->data_block_offset + parent_inode.block[9]*BLOCK_SIZE + offset_in_indirect_block ;
			move_last_dir_entry(last_offset,dir_entry_offset) ;
			unset_data_block_bit(parent_inode.block[9]) ;
		}
		else {
			last_offset = super_block->data_block_offset + (*(indirect_pointer_ptr+indirect_block_no))*BLOCK_SIZE + offset_in_indirect_block ;
			move_last_dir_entry(last_offset,dir_entry_offset) ;
			if(remove_last_data_block) {
				u32 idx = *(indirect_pointer_ptr+indirect_block_no+1) ;
				unset_data_block_bit(idx) ;
			}
		}
	}
	else { 
		//double indirect pointer not imlpemented rn
	}
	parent_inode.size -= sizeof(struct directory_entry) ;
	write_inode(parent_inode,in.parent) ;
}

/*
* This is helper function ofremove_entry_from_directory
* This moves the last directory entry to the place where directory
* is removed to avoid empty entries in directory
*/
void move_last_dir_entry(u32 last_offset, u32 dir_entry_offset) {
	struct directory_entry dir_to_move ;
	fseek(disk,last_offset,SEEK_SET) ;
	fread((void*)&dir_to_move,sizeof(struct directory_entry), 1, disk) ;
	fseek(disk,dir_entry_offset,SEEK_SET) ;
	fwrite((void*)&dir_to_move,sizeof(struct directory_entry),1,disk) ;
}

/*
* This is helper function of remove_entry_from_directory
* This finds the offset of the file/directory to be removed 
* in its parent directory
* Return: the offset of the directory entry to be removed
*/
u32 get_dir_entry_offset(struct inode dir, u32 file_ino) {
	char buffer[BLOCK_SIZE] ;
	u32 read_size=0 ;
	u32 dir_entries_per_block = BLOCK_SIZE/sizeof(struct directory_entry) ;
	for(int i=0 ; i<10 ; i++) {
		u32 offset = super_block->data_block_offset + BLOCK_SIZE*dir.block[i] ;
		read_block(offset,buffer) ;
		struct directory_entry* ptr=(struct directory_entry*)buffer ;
		for(int j=0 ; j<dir_entries_per_block ; j++) {
			if(ptr->inode==file_ino) return offset + j*sizeof(struct directory_entry) ;
			ptr++ ;
			read_size += sizeof(struct directory_entry) ;
			if(read_size >= dir.size) return -1;
		}
		read_size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ;
	}

	char indirect_pointer_buffer[BLOCK_SIZE] ;
	u32 indirect_pointer_offset = super_block->data_block_offset + BLOCK_SIZE*dir.block[10] ;
	fseek(disk,indirect_pointer_offset,SEEK_SET) ;
	fread((void *)indirect_pointer_buffer,BLOCK_SIZE,1,disk) ;
	u32* offset_ptr=(u32*)indirect_pointer_buffer; 
	for(int i=0 ; i<BLOCK_SIZE/sizeof(u32); i++) {
		u32 offset = super_block->data_block_offset + BLOCK_SIZE*(*offset_ptr) ;
		read_block(offset,buffer) ;
		struct directory_entry* ptr=(struct directory_entry*)buffer ;
		for(int j=0 ; j<dir_entries_per_block ; j++) {
			// printf("%s\n",ptr->name) ;
			if(ptr->inode==file_ino) return offset + j*sizeof(struct directory_entry) ;
			//  return 10 + i*dir_entries_per_block + j ; 
 			ptr++ ;
			if(read_size >= dir.size) return -1;
		}
		read_size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ;
		offset_ptr++ ;
	}

	//double indirect pointer not implemented rn
}

/*
* This clears all the data bits of the file indicated by inode in
*/
void clear_data_bits(struct inode in) {
	u32 file_size = in.size , size_read=0, i=0 ;
	while(size_read<file_size) {
		if(i<10) {
			unset_data_block_bit(in.block[i]) ;
			size_read += BLOCK_SIZE ;
		}
		else if(i<10+BLOCK_SIZE/sizeof(u32)) {
			char indirect_pointer[BLOCK_SIZE] ;
			u32 offset = super_block->data_block_offset + in.block[10]*BLOCK_SIZE ;
			read_block(offset,indirect_pointer) ;
			u32 *indirect_pointer_ptr = (u32*)indirect_pointer ;
			for(int j=0 ; j<BLOCK_SIZE/sizeof(u32) && size_read<file_size; j++) {
				unset_data_block_bit(*indirect_pointer_ptr) ;
				size_read += BLOCK_SIZE ;
			}
 		}
		else {
			char double_indirect_pointer[BLOCK_SIZE] ;
			u32 double_offset = in.block[11]*BLOCK_SIZE + super_block->data_block_offset ;
			read_block(double_offset,double_indirect_pointer) ;
			u32* double_indirect_pointer_ptr = (u32*)double_indirect_pointer ;
			for(int j=0 ; j<BLOCK_SIZE/sizeof(u32) && size_read<file_size ; j++) {
				u32 indirect_pointer_offset = super_block->data_block_offset + (*double_indirect_pointer_ptr)*BLOCK_SIZE ;
				double_indirect_pointer_ptr++ ;
				char indirect_pointer[BLOCK_SIZE] ;
				read_block(indirect_pointer_offset,indirect_pointer) ;
				u32* indirect_pointer_ptr = (u32*)indirect_pointer ;
				for(int k=0 ; k<BLOCK_SIZE/sizeof(u32) && size_read<file_size; k++) {
					unset_data_block_bit(*indirect_pointer_ptr) ;
					size_read+=BLOCK_SIZE ;
				}
			}
		}
	}
}

/*
* This removes a directory
*/
void delete_directory(const char* dirname) {
	u32 dir_inode = find_directory_or_file(dirname,0) ;
	struct inode dir = read_inode(dir_inode) ;
	delete_directory_files(dir,dirname) ;
	// clear_directory_data_bits(dir) ;
	delete_file(dirname) ;
}

// void clear_directory_data_bits(struct inode dir) {
// 	u32 read_size=0 ;
// 	for(int i=0 ; i<10 && read_size<dir.size ; i++) {
// 		unset_data_block_bit(dir.block[i]) ;
// 		read_size += BLOCK_SIZE ;
// 	}
// 	char indirect_pointer[BLOCK_SIZE] ;
// 	u32 indirect_pointer_offset = super_block->data_block_offset + dir.block[10]*BLOCK_SIZE ;
// 	read_block(indirect_pointer_offset,indirect_pointer) ;
// 	u32* indirect_pointer_ptr = (u32*) indirect_pointer ;
// 	for(int i=0 ; i<BLOCK_SIZE/sizeof(u32) && read_size<dir.size; i++) {
// 		u32 data_block_idx = *indirect_pointer_ptr ;
// 		indirect_pointer_ptr++; 
// 		unset_data_block_bit(data_block_idx) ;
// 		read_size+=BLOCK_SIZE ;
// 	}
// 	// indirect pointer not implemented rn
// }

/*
* This is helper function of delete_directory
* This deletes all the directory entries of a directory one by one
* TODO: implementation of double indirect pointer
*/
void delete_directory_files(struct inode dir, const char* dirname) {
	u32 read_size = 0 ; 
	char buffer[BLOCK_SIZE] ;
	u32 dir_entries_per_block = BLOCK_SIZE/sizeof(struct directory_entry) ;
	for(int i=0 ; i<10 ; i++) {
		u32 offset = super_block->data_block_offset + BLOCK_SIZE*dir.block[i] ;
		read_block(offset,buffer) ;
		struct directory_entry* ptr=(struct directory_entry*)buffer ;
		char filename[MAX_FILE_NAME_LEN] ;
		for(int j=0 ; j<dir_entries_per_block ; j++) {
			// printf("%s\n",ptr->name) ;
			strcpy(filename,dirname) ;
			strcat(filename,"/") ;
			strcat(filename,ptr->name) ;
			delete_file(filename) ;
			ptr++ ;
			read_size += sizeof(struct directory_entry) ;
			if(read_size >= dir.size) return ;
		}
		read_size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ;
	}

	char indirect_pointer_buffer[BLOCK_SIZE] ;
	u32 indirect_pointer_offset = super_block->data_block_offset + BLOCK_SIZE*dir.block[10] ;
	fseek(disk,indirect_pointer_offset,SEEK_SET) ;
	fread((void *)indirect_pointer_buffer,BLOCK_SIZE,1,disk) ;
	u32* offset_ptr=(u32*)indirect_pointer_buffer; 
	for(int i=0 ; i<BLOCK_SIZE/sizeof(u32); i++) {
		u32 offset = super_block->data_block_offset + BLOCK_SIZE*(*offset_ptr) ;
		read_block(offset,buffer) ;
		struct directory_entry* ptr=(struct directory_entry*)buffer ;
		for(int j=0 ; j<dir_entries_per_block ; j++) {
			// printf("%s\n",ptr->name) ;
			delete_file(ptr->name) ;
 			ptr++ ;
			if(read_size >= dir.size) return ;
		}
		read_size += BLOCK_SIZE - dir_entries_per_block*sizeof(struct directory_entry) ;
		offset_ptr++ ;
	}

	//double indirect pointer not implemented rn

}

/*
* This closes the file system
*/
void close_filesystem() {
	fseek(disk,0L,SEEK_SET) ;
	fwrite((void*)super_block,sizeof(struct super_block),1,disk) ;
	fclose(disk) ;
}

/*****************************************************************************************************/

/* BELOW ARE THE FUCNTIONS USED TO TEST THE FILE SYSTEM */

u8 check_inode_bit(u32 ino) {
	fseek(disk,super_block->inode_bitmap_offset+ino/8,SEEK_SET) ;
	u8 byte ;
	fread((void*)&byte,1,1,disk) ;
	printf("inode bit %d : %d\n",ino,byte) ;
	return byte ; 
}

u8 check_data_bit(u32 data_bit) {
	fseek(disk,super_block->data_block_bitmap_offset+data_bit/8,SEEK_SET) ;
	u8 byte ;
	fread((void*)&byte,1,1,disk) ;
	printf("data bit %d : %d\n",data_bit,byte) ;
	return byte ;
}

void status_superblock() {
	fseek(disk,0L,SEEK_SET) ;
	struct super_block superblock ;
	fread((void*)&superblock,sizeof(struct super_block),1,disk) ;
	printf("block size = %d\n",superblock.block_size) ;
	printf("total block count size = %d \n",superblock.total_block_count) ;
	printf("inode count = %d \n",superblock.inode_count) ;
	printf("inode bitmap offset = %d \n",superblock.inode_bitmap_offset) ;
	printf("data block bitmap offset = %d \n",superblock.data_block_bitmap_offset) ;
	printf("inode table offset = %d \n",superblock.inode_table_offset) ;
	printf("data block offset = %d \n",superblock.data_block_offset) ;
	printf("free inode count = %d \n",superblock.free_inode_count) ;
	printf("free data block count = %d \n",superblock.free_data_block_count) ;
	printf("root directory inode = %d \n",superblock.root_directory_inode) ;
	printf("inode size = %d \n",superblock.inode_size) ; 
}

void print_inode(u16 ino) {
	struct inode in = read_inode(ino) ;
	printf("mode = %d\n",in.mode[0]) ;
	printf("size of file/directory = %d\n",in.size) ;
	printf("parent inode = %d\n",in.parent) ;
	in = read_inode(in.parent) ;
	printf("inode parent size = %d\n",in.size) ;
}

void print_dir_entry(u32 offset) {
	fseek(disk,offset,SEEK_SET) ;
	struct directory_entry dir ;
	fread((void*)&dir,sizeof(struct directory_entry),1,disk) ;
	printf("inode = %d\n",dir.inode) ;
	printf("name = %s\n",dir.name) ;
}
