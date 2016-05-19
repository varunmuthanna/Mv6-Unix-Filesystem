/**
Author : Varun Muthanna
	 Sumit Jain
**/


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "struct.h"

#define BLOCK_SZ 512
#define MAX_FILE_SIZE 32*1024*1024

superblock Super;
inode Inode;
inode root_Inode;

int fd, total_num_blocks, total_num_blocks, total_data_blocks;
int total_inode_blocks, total_inodes, num_of_freelist_block;
int root_inodenum;

int num_of_free_blocks_available()
{
	superblock Super;
	int num_of_free_blocks = 0;
	unsigned short nfree = 0, first_block_num = 1;

	lseek(fd, BLOCK_SZ, SEEK_SET);

	read(fd, &Super, sizeof(Super));

	num_of_free_blocks = Super.nfree;

	if(Super.free[0] != 0)
		lseek(fd, BLOCK_SZ * Super.free[0], SEEK_SET);
	else
		return num_of_free_blocks;

	while(first_block_num != 0)
	{
		read(fd, &nfree, sizeof(nfree));
		num_of_free_blocks += nfree;		

		read(fd, &first_block_num, sizeof(nfree));
		lseek(fd, first_block_num * BLOCK_SZ, SEEK_SET);
	}

	return num_of_free_blocks;
}

debug()
{
	int i, j;
	unsigned short nfree, free_block, first_block;

	lseek(fd, BLOCK_SZ, SEEK_SET);

	read(fd, &Super, sizeof(Super));

	printf("\n(Super Block) isize = %d\n", Super.isize);
	printf("(Super Block) fsize = %d\n", Super.fsize);
	printf("(Super Block) nfree = %d\n", Super.nfree);

	printf("\n-----Free block list in super block-----\n");
	for(i = 0; i < Super.nfree; i++)
		printf(" %d", Super.free[i]);
	printf("\n");

	lseek(fd, 2*BLOCK_SZ, SEEK_SET);
	printf("total inodes = %d \n", total_inodes);
	for(i = 0; i < total_inodes; i++)
	{
		read(fd, &Inode, sizeof(Inode));
		if(Inode.flags & 0x8000)
			printf("\nInode - %d is allocated\n\n", i);
	}

	printf("-----Printing the free list-----\n");

	lseek(fd, Super.free[0] * BLOCK_SZ, SEEK_SET);


	for(i = 0; i < num_of_freelist_block; i++)
	{
		read(fd, &nfree, 2);

		if(nfree != 0)
		{
			printf("nfree = %d ", nfree);
			for(j = 0; j < nfree; j++)
			{
				read(fd, &free_block, sizeof(free_block));

				if(j == 0)
					first_block = free_block;

				printf(" %d", free_block);
			}

			if(nfree != 99)
				break;

			if((i+1) < num_of_freelist_block)
				printf("\n\n-----Going to the next free list block-----\n ");
			lseek(fd, first_block * BLOCK_SZ, SEEK_SET);
		}
	}

	printf("\n\n");
}

initialize_super_struct()
{
	Super.isize = total_inode_blocks;
	Super.fsize = total_num_blocks;
	Super.nfree = 100;
}

initialize_inode_structs()
{
	memset((void *) &Inode, 0, sizeof(Inode));
}

Initialize_all_metadata(char * path)
{
	fd = open(path, O_RDWR);

	if(fd < 0)
	{
		printf("Cannot open the file %s\n", path);
		exit(0);
	}

	/* Initializing SUPER structures */
	lseek(fd, BLOCK_SZ, SEEK_SET);
	read(fd, &Super, sizeof(Super));

	/* Initializing ROOT INODE structure */
	lseek(fd, BLOCK_SZ*2, SEEK_SET);
	read(fd, &root_Inode, sizeof(inode));

	total_inode_blocks = Super.isize;
}

void initialize_root_inode_struct()
{
	int i;

	root_Inode.flags = 0xC006;
        root_Inode.nlinks = 0;
        root_Inode.uid = 0;
        root_Inode.gid = 0;
        root_Inode.size0 = 0;
        root_Inode.size1 = 32;

	root_Inode.addr[0] = 1 + 1 + total_inode_blocks; /* boot_block + super_block + total_inode_block + 1 */

        for(i = 1; i < 8; i++)
        {
                root_Inode.addr[i] = 0;
        }

	/* TO DO */
        for(i = 0; i < 2; i++)
        {
                root_Inode.actime[i] = 0;
                root_Inode.modtime[i] = 0;
        }
}

initialize_free_list_blocks()
{
	unsigned short nfree = 100;
	unsigned short free_data_block_num, free_list_block_num;

	int i, j;
        unsigned short null_block = 0;
	int total_data_blocks = total_num_blocks - total_inode_blocks - 2;
	int total_free_data_blocks = (total_data_blocks - 1 - 100);

	/* num_of_freelist_block = total_data_blocks - root data block */
	num_of_freelist_block = ((total_data_blocks - 100 - 1) / 100 ) + 1; 


	lseek(fd, (2 + total_inode_blocks + 1) * BLOCK_SZ, SEEK_SET);

	free_list_block_num = 2 + total_inode_blocks + 1;

	free_data_block_num = free_list_block_num + num_of_freelist_block;

	Super.free[0] = free_list_block_num;

	for(i = 1; i < 100; i++)
	{
		Super.free[i] = free_data_block_num++;
	}

	/* Free data block list */
	for(i = 0; i < num_of_freelist_block; i++)
	{

		if(total_free_data_blocks / 100)
			nfree = 100 - 1;
		else
			nfree = total_free_data_blocks % 100;
		write(fd, &nfree, 2);
		if(i == num_of_freelist_block - 1)
		{
			write(fd, &null_block, 2);
		}
		else
		{
			free_list_block_num++;
			write(fd, &free_list_block_num, 2);
			total_free_data_blocks--;
		}

		for(j = 0; j < nfree - 1; j++)
		{
			write(fd, &free_data_block_num, 2);
			free_data_block_num++;
			total_free_data_blocks--;
		}

		lseek(fd, free_list_block_num*BLOCK_SZ, SEEK_SET);
	}
}

/**
    Once the free data blocks are over in the Super block it is filled up
   from the global free data block list
   return : 1 if successfully allocated to super block
          : 0 if the data blocks in global free list is over.
*/
int reallocate_freelist()
{
        unsigned short free_list_block_num;
        unsigned short nfree;
        unsigned short block_num;

        int i;
	/* The first element of the Super block free list points to
	  the data block containg the global free data block list
	  the final block of free data block list is initialize to 0 just to initimate
	 the end of free list*/
        if(Super.free[0] == 0)
        {
                printf("No more data blocks are available for allocation \n");
                return 0;
        }
        else
        {
		/* The global free list is initialized such that the first two 
		  bytes gives the number of free data block numbers held in the 
		  present data block and the first datablock number will contains the 
		  the next data block number holding the free data block list */
                free_list_block_num = Super.free[0];
                lseek(fd, BLOCK_SZ * free_list_block_num, SEEK_SET);
                read(fd, &nfree, 2);
                for(i = 0; i < nfree; i++)
                {
                        read(fd, &block_num, 2);
                        Super.free[i] = block_num;
                }
                Super.free[i] = free_list_block_num;
                Super.nfree = nfree + 1;
                return 1;
        }
}

/** get a free data block from the free list in super block
   When the free data block are used up in the Super block, it is
   filled up from the free list. 
*/
int get_free_data_block()
{
        if(Super.nfree == 1)
        {
                if(reallocate_freelist() != 1)
		{
                        return 0;
		}
		else
		{
			Super.nfree--;
                	return Super.free[Super.nfree];
		}
        }
        else
        {
		Super.nfree--;
                return Super.free[Super.nfree];
        }
}

/** Enter the filename to the directory's file list.
    filename : string of the filename to be entered into the directory's file list
    file_inode_num : inode number of the file filename
    dir_inode_num : inode number of the directory.
*/
int entry_filename_dir(char *filename, unsigned short dir_inode_num, unsigned short file_inode_num)
{
	int i = 0, j = 0, k = 0, l = 0;
	int inode_pos;
	unsigned short inode_num;
	unsigned short data_blocknum, data_blocknum_2, free_data_block;
	unsigned short inode_addr_data_block, addr_data_block;
	unsigned short data_block, data_block_1, data_block_2;
	unsigned short zero_block = 0;
	int addr_changed = 0;
	inode dir_inode;
	inode_pos = (BLOCK_SZ * 2) + (dir_inode_num * sizeof(inode));
	lseek(fd, inode_pos, SEEK_SET);
	read(fd, &dir_inode, sizeof(inode));
	if(!(dir_inode.flags & 0x4000))
	{
		printf("Entered name is not a directory \n");
		return 0;
	}
	if(!(dir_inode.flags & 0x1000))           //not a large directory
	{
		for(i = 0; (dir_inode.addr[i] != 0) && (i < 7); i++);

		if((i == 7) && (dir_inode.addr[i] != 0))
		{
			data_blocknum = dir_inode.addr[i];
		}
		else
		{
			data_blocknum = dir_inode.addr[--i];   //consider the previous valid block
		}
		if(i > 0)
		{
			j = 0;
			lseek(fd, BLOCK_SZ * data_blocknum, SEEK_SET);  
		}
		else
		{
			j = 2;
			lseek(fd, BLOCK_SZ * data_blocknum + 32, SEEK_SET);  
		/* +32 is for the 16 bytes which is holding the names of self and parent names */
		}
		for( ; j < 32; j++)
		{
			read(fd, &inode_num, 2);
			if(inode_num == 0)
			{
				lseek(fd, -2, SEEK_CUR);
				write(fd, &file_inode_num, 2);
				write(fd, filename, 14);
				break;
			}
			else
			{
				lseek(fd, 14, SEEK_CUR);
				continue;
			}
		}
		if(j != 31)
		{
			write(fd, &zero_block, 2);
		}
		else
		{
			if(i < 7)
			{
				dir_inode.addr[i+1] = get_free_data_block();
				lseek(fd, BLOCK_SZ *  dir_inode.addr[i+1], SEEK_SET);
				write(fd, &zero_block, 2);
			}
			else
			{
				dir_inode.flags |= 0x1000;
				data_blocknum_2 = get_free_data_block();
				if(data_blocknum_2 == 0)
				{
					printf("out of data blocks \n");
					return 0;
				}
				free_data_block = get_free_data_block();
                                if(free_data_block == 0)
                                {
                                        printf("out of data blocks \n");
                                        return 0;
                                }
				lseek(fd, data_blocknum_2 * BLOCK_SZ, SEEK_SET);
				for(k = 0; k < 8; k++)
				{
					data_block = dir_inode.addr[k];
                                        dir_inode.addr[k] = 0;
                                        write(fd, &data_block, 2);
				}
				write(fd, &free_data_block,2);
				write(fd, &zero_block, 2);
                                dir_inode.addr[0] = data_blocknum_2;
			}
		}
	}
	else
	{
		for(i = 0; (dir_inode.addr[i] != 0) && (i < 7); i++);

                if((i == 7) && (dir_inode.addr[i] != 0))
                {
                        addr_data_block = dir_inode.addr[i];
			for(l = 0; l < BLOCK_SZ/2; l++)
			{
				lseek(fd, (addr_data_block* BLOCK_SZ) + (l * 2), SEEK_SET);
				read(fd, &data_blocknum, 2);
				if(data_blocknum == 0)
				{
					lseek(fd, -4, SEEK_CUR);
                                	read(fd, &data_blocknum, 2 );
				}
				
			}
                }
                else
                {
                        data_blocknum = dir_inode.addr[--i];   //consider the previous valid block
                }
		for(j = 0; j < BLOCK_SZ/2; j++)
		{
			lseek(fd, data_blocknum * BLOCK_SZ + (j * 2), SEEK_SET);
			read(fd, &data_blocknum_2, 2);
			if((data_blocknum_2 == 0) || (j == BLOCK_SZ/2 - 1) )
			{
				lseek(fd, -4, SEEK_CUR);
				read(fd, &data_block_2, 2 );
				for(k = 0; k < BLOCK_SZ/16; k++)
				{
					lseek(fd, data_block_2 * BLOCK_SZ + k * 16, SEEK_SET);
					read(fd, &inode_num, 2);
					if(inode_num == 0)
                        		{
                                		lseek(fd, -2, SEEK_CUR);
                                		write(fd, &file_inode_num, 2);
                                		write(fd, filename, 14);
                                		break;
                        		}
                        		else
                        		{
                                		lseek(fd, 14, SEEK_CUR);
                               	 		continue;
                        		}
				}
				if(k != 31)
				{
					write(fd, &zero_block, 2);
					break;
				}
				else
				{
					data_block = get_free_data_block();
                        		lseek(fd, BLOCK_SZ *  data_block, SEEK_SET);
                        		write(fd, &zero_block, 2);
				}

				if(j == BLOCK_SZ/2 - 1)
				{
					inode_addr_data_block = get_free_data_block();
					lseek(fd, inode_addr_data_block * BLOCK_SZ, SEEK_SET);
					if(i == 6)
					{
						data_block_1 = get_free_data_block();
						write(fd, &data_block_1, 2);
						write(fd, &zero_block, 2);
						lseek(fd, data_block_1 * BLOCK_SZ, SEEK_SET);
						write(fd, &data_block, 2);
						write(fd, &zero_block, 2);	
					}
					else
					{
						write(fd, &data_block, 2);
						write(fd, &zero_block, 2);
					}
					addr_changed = 1;
				}
				else
				{
					lseek(fd, data_blocknum * BLOCK_SZ + (j * 2), SEEK_SET);
					write(fd, &data_block, 2);
					write(fd, &zero_block, 2);
				}
			}
		}
		if(addr_changed)
		{
			if(i < 7)
			{
				dir_inode.addr[i] = inode_addr_data_block;
			}
			else
			{
				lseek(fd, addr_data_block * BLOCK_SZ + (l * 2), SEEK_SET);
				write(fd, &inode_addr_data_block,2);
				if(l < BLOCK_SZ/2 - 1)
				{
					write(fd, &zero_block, 2);
				}
				else
				{
					printf("out of address space for entering the file names \n");
					return 0;
				}
			}
			addr_changed = 0;
		}
	}
	lseek(fd, inode_pos, SEEK_SET);
        write(fd, &dir_inode, sizeof(inode));
	return 1;
}

/** get the file name from the entered file path.
*/
get_filename(char *file_path, char *filename)
{
	int i = 0, j = 0;
	while(file_path[i] != '\0')
	{
		if(file_path[i] != '/')
		{
			printf("path of the external path is not right \n");
			filename = NULL;
			return;
		}
		i++;
		for(j = 0 ;(file_path[i] != '/') ;i++,j++)
		{
			if(file_path[i] == '\0'){
			break;
			}
			filename[j] = file_path[i]; 
			if(i > 100)
			break;
		}
		if(file_path[i] == '\0')
		{
			filename[j] = file_path[i];
			return;
		}
		else
		{
			continue;
		}
	}
}

/** Initialize the root data block
*/
initialize_root_data_block()
{
	unsigned short inode_num = 0;
	unsigned short zero_block = 0;
	char name1[14] = ".";
	char name2[14] = "..";

	lseek(fd, ((2 + total_inode_blocks)*BLOCK_SZ), SEEK_SET);

	write(fd, &inode_num, 2);
	write(fd, name1, 14);
	write(fd, &inode_num, 2);
	write(fd, name2, 14);
	write(fd, &zero_block, 2);
}

/** Scan through the inode list to get the unallocated inode */
int get_free_inode()
{
	int i;
	inode temp_inode;
	int inode_pos;
	ssize_t read_size;
	inode_pos = BLOCK_SZ * 2;
	lseek(fd, inode_pos + sizeof(inode), SEEK_SET);  // +32 to exclude root inode
	for(i = 1; i < Super.isize * 16; i++)   // incase the number of inodes is multiple of 16
	{
		read_size = read(fd, &temp_inode, sizeof(inode));
		if(read_size == -1)      // incase the number of inodes is not a multiple of 16
		{
			printf("No more free inodes \n");
			return 0;
		}
		if(!(temp_inode.flags & 0x8000))
		{
			return i;
		}
	}
	
}

/** Create a file inside the filesystem. Opens the external file
   and copies block by block into the filesystem.
   ext_path : The path of the external file which is to be copied.
   dir_inode_num : Inode number of the directory into which the file needs to be created.
   return : 0 if the create file fails 
          : Inode number of the file successfully created
*/
int create_file(char *ext_path, int dir_inode_num)
{
	int ext_fd;
	int i = 0, j = 0, k = 0, l = 0;
	int direct_addressing = 1;
	int indirect_addressing = 0;
	int double_indirect_addr = 0;
	unsigned int size = 0;
	char ext_data[512];
	ssize_t read_size, write_size;
	unsigned short data_block_num;
	unsigned short data_block, data_block_2, data_block_3, data_block_test, data_block_2_test;
	unsigned short zero_block = 0;
	int inode_num;
	inode file_inode;
	memset(&file_inode, 0, sizeof(inode));
	file_inode.flags |= 0x8000;
	ext_fd = open(ext_path, O_RDONLY);
	while(1)
	{
		read_size = read(ext_fd, ext_data, 512);
		if(read_size == 0)
		{
			break;
		}
		else
		{
			size += read_size;
			if(size > MAX_FILE_SIZE)
			{
				printf("Cannot copy files larger than 32 MB \n");
				break;       // TODO : Add the blocks used back to free list
			}
			if(!indirect_addressing && !direct_addressing && !double_indirect_addr)
			{
				indirect_addressing = 1;
				data_block_num = get_free_data_block();
				lseek(fd, data_block_num * BLOCK_SZ, SEEK_SET);
				for(j = 0; j < 8; j++)
				{
					data_block = file_inode.addr[j];
					file_inode.addr[j] = 0;
					write(fd, &data_block, 2);
				}
				i = 0;
				k = 8;
				write(fd, &zero_block, 2);
				file_inode.addr[i] = data_block_num;
				file_inode.flags |= 0x1000;
			}
			data_block_num = get_free_data_block();
			if(data_block_num == 0)
			{
				printf("out of data blocks \n");
				return 0;
			}
			lseek(fd, data_block_num * BLOCK_SZ, SEEK_SET);
			write_size = write(fd, ext_data, read_size);
			if(direct_addressing)
			{
				file_inode.addr[i++] = data_block_num;
				if(i > 7)
				{
					direct_addressing = 0;
				}
			}
			if(indirect_addressing)
			{
				data_block = file_inode.addr[i];
				lseek(fd, data_block * BLOCK_SZ +(k * 2), SEEK_SET);
				write(fd, &data_block_num, 2);
				k++;
				if(k > 255)
				{
					data_block = get_free_data_block();
					if(data_block == 0)
					{
						printf("Out of data blocks \n");
						return 0;
					}
					lseek(fd, data_block * BLOCK_SZ, SEEK_SET);
					i++;
					file_inode.addr[i] = data_block;
					if(i == 7)
					{
						indirect_addressing = 0;
						double_indirect_addr = 1;
						data_block_2 = get_free_data_block();
						lseek(fd, data_block * BLOCK_SZ, SEEK_SET);
						write(fd, &data_block_2, 2);
						write(fd, &zero_block, 2);
						lseek(fd, data_block_2 * BLOCK_SZ,SEEK_SET);
						write(fd, &zero_block, 2);
						l = 0;
						k = 0; 
						continue;
					}
					else
					{
						write(fd, &zero_block, 2);
					}
					k = 0;
				}
				else
				{
					write(fd, &zero_block, 2);
				}
			}   
			if(double_indirect_addr)
			{
				data_block = file_inode.addr[i];
                                lseek(fd, data_block * BLOCK_SZ +(k * 2), SEEK_SET);
				read(fd, &data_block_2, 2);
				lseek(fd, data_block_2 * BLOCK_SZ + (l * 2), SEEK_SET);
				write(fd, &data_block_num, 2);
				l++;
				if(l > 255)
				{
					k++;
					if(k > 255)
					{
						printf("out of the space in the indirect address \n");
						return 0;
					}
					data_block_3 = get_free_data_block();
                                        if(data_block_3 == 0)
                                        {
                                                printf("Out of data blocks \n");
                                                return 0;
                                        }
                                        lseek(fd, data_block_3 * BLOCK_SZ, SEEK_SET);
					write(fd, &zero_block, 2);
					lseek(fd, data_block * BLOCK_SZ +(k * 2), SEEK_SET);
					write(fd, &data_block_3, 2);
					write(fd, &zero_block, 2);
					l = 0;
				}
				else
				{
					write(fd, &zero_block, 2);
				}
			}
		}
	}
	file_inode.size1 = size & 0x0000FFFF;
	file_inode.size0 = (size >> 16) & 0x00FF;
	if(size > (16*1024*1024))
	{
		file_inode.flags |= 0x0200;
	}

	/* Insert the permission */
	file_inode.flags |= 0x1B6; /* Read and Write permissions to Owner, Group and Others */

	inode_num = get_free_inode();    // inode number recieved is considering root node as 0
	lseek(fd, BLOCK_SZ*2 + inode_num * sizeof(inode), SEEK_SET);

	write(fd, &file_inode, sizeof(inode));
	lseek(fd, BLOCK_SZ, SEEK_SET);
        write(fd, &Super, sizeof(Super));
	return inode_num;
}

/*  Find the inode of the file or directory within certain
    directory.
    char *directory : Name of the directory whose inode to be found
    dir_inode_num   : Inode number of the *directory in which directory is present
    is_dir          : Is *directory a directory or a file
    return value    : Inode number of the *directory 
*/
int find_dirInode(char *directory, int dir_inode_num, int is_dir)
{
	int i = 0, j = 0, k = 0, l = 0;
	int inode_pos;
	unsigned short new_dir_inode_num, data_block_num, data_block, data_block_1;
	char dir_name[14];
	inode dir_inode;
	inode_pos = BLOCK_SZ *2 + dir_inode_num * 32;
	lseek(fd, inode_pos, SEEK_SET);
	read(fd, &dir_inode, sizeof(inode));
	if((!(dir_inode.flags & 0x4000)) && is_dir)
	{
		printf("the entered directory %s is not a directory \n",directory);
                return 0;  //error
	}

	if(!(dir_inode.flags & 0x1000))    //not a large directory (direct addressing)
	{
		for(i = 0; i < 8 ; i++)
		{
			if(dir_inode.addr[i] == 0)
			{
				return 0;
			}
			data_block_num = dir_inode.addr[i];
			lseek(fd, BLOCK_SZ * data_block_num, SEEK_SET);
			if(i == 0)
			{
				lseek(fd, 32, SEEK_CUR);      //first 32 bytes are for itself and parent dir
			}
			for(j = 0; j < ((BLOCK_SZ/16) - 2); j++)
			{
				read(fd, &new_dir_inode_num, 2);
				if(new_dir_inode_num != 0)
				{
					read(fd, dir_name, 14);
					if(strcmp(dir_name, directory) == 0)
					{
						return new_dir_inode_num; 
					}
				}
				else
				{
					return 0;
				}
			}
		}
	}
	else
	{
		for(i = 0; i < 7 ; i++)
                {
                        if(dir_inode.addr[i] == 0)
                        {
                                return 0;
                        }
                        data_block_num = dir_inode.addr[i];
			for(k = 0; k < BLOCK_SZ/2; k++)
			{
				lseek(fd, data_block_num * BLOCK_SZ + (k * 2), SEEK_SET);
				read(fd, &data_block, 2);
				if(data_block == 0)
				{
					return 0;
				}
				lseek(fd, data_block * BLOCK_SZ, SEEK_SET);
				j = 0;
				if(i == 0 && k == 0)
                        	{
                                	lseek(fd, 32, SEEK_CUR);      //first 32 bytes are for itself and parent dir
					j = 2;
                        	}

				for(; j < ((BLOCK_SZ/16)); j++)
                        	{
                                	read(fd, &new_dir_inode_num, 2);
                                	if(new_dir_inode_num != 0)
                                	{
                                        	read(fd, dir_name, 14);
                                        	if(strcmp(dir_name, directory) == 0)
                                        	{
                                                	return new_dir_inode_num;
                                        	}
                                	}
                                	else
                                	{
                                        	return 0;
                                	}
                        	}
			}
		}

		if(i == 7)
                {
                        data_block_1 = dir_inode.addr[i];
                        if(data_block_1 == 0)
                        {
                                return 0;
                        }
                        for(l = 0; l < BLOCK_SZ/2; l++)
                        {
                                lseek(fd, data_block_1 * BLOCK_SZ + l * 2, SEEK_SET);
                                read(fd, &data_block_num,2);
                                if(data_block_num == 0)
                                {
                                        return 0;
                                }

                                for(k = 0; k < BLOCK_SZ/2; k++)
                                {
                                        lseek(fd, data_block_num * BLOCK_SZ + (k * 2), SEEK_SET);
                                        read(fd, &data_block, 2);
                                        if(data_block == 0)
                                        {
                                                return 0;
                                        }
                                        lseek(fd, data_block * BLOCK_SZ, SEEK_SET);
                                        for(j = 0; j < ((BLOCK_SZ/16)); j++)
                                        {
                                                read(fd, &new_dir_inode_num, 2);
                                                if(new_dir_inode_num != 0)
                                                {
                                                        read(fd, dir_name, 14);
							if(strcmp(dir_name, directory) == 0)
                                                        {
                                                                return new_dir_inode_num;
                                                        }
                                                }
                                                else
                                                {
                                                        return 0;;
                                                }
                                        }
                                }
                        }
                }
	}
        return 0;	
}

initfs(char *path)
{
	int i;

	if( access( path, F_OK ) != -1 ) /* If File already exists */
		printf("File already exists\nReinitializing the same filesystem again\n");
	

		fd = open(path, O_RDWR | O_CREAT, 0666);
	if(fd < 0)
	{
		printf("Error: Cannot create file. Please enter appropriate path\n");
		exit(0);
	}

	if(total_inodes % 16)
                total_inode_blocks = (total_inodes / 16) + 1;
        else
                total_inode_blocks = (total_inodes / 16);

	initialize_root_inode_struct();
	initialize_inode_structs();
	
	lseek(fd, BLOCK_SZ*2, SEEK_SET);
	write(fd, &root_Inode, sizeof(inode));

	lseek(fd, BLOCK_SZ*2 + sizeof(root_Inode), SEEK_SET);

	for(i = 0; i < (total_inodes - 1); i++)
	{
		write(fd, &Inode, sizeof(Inode));
	}

	initialize_root_data_block();

	initialize_free_list_blocks(total_num_blocks,total_inode_blocks);

	initialize_super_struct();

	lseek(fd, BLOCK_SZ, SEEK_SET);
	write(fd, &Super, sizeof(Super));

}

/* Copies the file present in the cretain mentioned directory 
   to the external file.
  filename : name of the file to be copied 
  ext_file : The path to which the file needs to be copied to.
  dir_inode_num : Inode number of the directory in which file of name
                  filename is persent
*/
int copy_file_ext(char *filename,char *ext_file, unsigned short dir_inode_num)
{
	int i = 0,j = 0, k = 0;
	unsigned short fileInode_num;
	int inode_pos;
	inode fileInode;
	int data_block_num;
	unsigned short data_block_1, data_block_2,data_block_3;
	int ext_fd;
	unsigned int size;
	ssize_t read_size, write_size, test_size;
	char data[512]; 
	fileInode_num = find_dirInode(filename, dir_inode_num,0);
	if(fileInode_num == 0)
	{
		printf("Could not find the Inode of %s \n",filename);
		return 0;
	}
	inode_pos = (BLOCK_SZ * 2) + (fileInode_num * sizeof(inode));
	lseek(fd, inode_pos, SEEK_SET);
	read(fd, &fileInode, sizeof(inode));

        if(ext_file[i] != '/')
        {
        	printf("path of the external path is not right \n");
                return 0;
        }
        i++;
	for(;ext_file[i] != '\0'; i++ );
	if(ext_file[i - 1] == '/')
	{
		for(j = 0; filename[j] != '\0'; j++)
		{
			ext_file[i++] = filename[j];
		}
		ext_file[i] = filename[j];
	}

	ext_fd = open(ext_file, O_TRUNC | O_CREAT | O_RDWR, 0666);
	if(ext_fd < 0)
	{
		printf("Could not open/create external file \n");
		return 0;
	}
	size = fileInode.size0 & 0xFF;
	size = (size << 16) + fileInode.size1;
	if(fileInode.flags & 0x0200)
	{
		size |= 0x01000000;
	}
	printf("copy to file : size = %d \n",size);
	if(!(fileInode.flags & 0x1000))
	{
		for(i = 0; i < 8; i++)
		{
			if(fileInode.addr[i] == 0)
			{
				break;
			}
			

			data_block_num = fileInode.addr[i];
			lseek(fd, BLOCK_SZ * data_block_num, SEEK_SET);
			if(size > 512)
			{
				read_size = read(fd, &data, BLOCK_SZ);
				write_size = write(ext_fd, &data, read_size);
				size = size - write_size;
			}
			else
			{
				read_size = read(fd, &data, size);
                                write_size = write(ext_fd, &data, read_size);
			}
		}
	}
	else
	{
		for(i = 0; i < 8; i++)
                {
                        if(fileInode.addr[i] == 0)
                        {
                                break;
                        }

			if(i < 7)
			{
				data_block_1 = fileInode.addr[i];
				for(j = 0; j < BLOCK_SZ/2; j++)
				{
					lseek(fd, (BLOCK_SZ * data_block_1) + (j * 2), SEEK_SET);
					read(fd, &data_block_2, 2);
					if(data_block_2 == 0)
					{
						break;
					}
					lseek(fd, data_block_2 * BLOCK_SZ, SEEK_SET);
					if(size > 512)
					{
						read_size = read(fd, &data, BLOCK_SZ);
						write_size = write(ext_fd, &data, read_size);
						size = size - write_size;
					}
					else
					{
						read_size = read(fd, &data, size);
						write_size = write(ext_fd, &data, read_size);
						size = size - write_size;
					}
				}
			}
			else
			{
				data_block_1 = fileInode.addr[i];
                                for(j = 0; j < BLOCK_SZ/2; j++)
                                {
                                        lseek(fd, (BLOCK_SZ * data_block_1) + (j * 2), SEEK_SET);
                                        read(fd, &data_block_2, 2);
					if(data_block_2 == 0)
                                        {
                                                break;
                                        }
					for(k = 0; k < BLOCK_SZ/2; k++)
					{
                                        	lseek(fd, (data_block_2 * BLOCK_SZ) + (k * 2), SEEK_SET);
						read(fd, &data_block_3, 2);
                                        	if(data_block_3 == 0)
                                        	{
                                                	break;
                                        	}
                                        	lseek(fd, data_block_3 * BLOCK_SZ, SEEK_SET);
                                        	if(size > 512)
                                        	{
                                                	read_size = read(fd, &data, BLOCK_SZ);
                                                	write_size = write(ext_fd, &data, read_size);
                                                	size = size - write_size;
                                        	}
                                        	else
                                        	{
                                                	read_size = read(fd, &data, size);
                                                	write_size = write(ext_fd, &data, read_size);
							size = size - write_size;
                                        	}
					}
				}
			}
                }
	}
	close(ext_fd);
	if(size == 0)
	{
		printf("copy completed. \n");
		return 1;
	}
	else
	{
		return 0;
	}
}

/** Creates a directory by initializing new inode and respective
   data block.
   directory : Name of the directory to be created
   dir_inode_num : Inode of the parent directory.
*/
int create_directory(char *directory, unsigned short dir_inode_num)
{
	unsigned short inode_num;
	inode new_dir_inode;
	char name1[14] = ".";
	char name2[14] = "..";
	unsigned short zero_block = 0;
	int error;

	memset(&new_dir_inode, 0, sizeof(inode));

	new_dir_inode.flags |= 0x4000;
	new_dir_inode.addr[0] = get_free_data_block();
	if(new_dir_inode.addr[0] == 0)
	{
		printf("Out of data blocks \n");
		return 0;
	}
	new_dir_inode.addr[1] = 0;	
	inode_num = get_free_inode();
	lseek(fd, new_dir_inode.addr[0] * BLOCK_SZ, SEEK_SET);

	write(fd, &inode_num, 2);
	write(fd, name1, 14);
	write(fd, &dir_inode_num, 2);
	write(fd, name2, 14);
	write(fd, &zero_block, 2);
	//TODO : to fill up other elements of the inode

	new_dir_inode.flags |= 0x8000;
	lseek(fd, BLOCK_SZ *2 +(inode_num *32), SEEK_SET);
	write(fd, &new_dir_inode, sizeof(inode));

	error =  entry_filename_dir(directory, dir_inode_num, inode_num);
	if(error == 0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/**
    Display all the files and directories inside given directory.
    dir_inode_num : Inode of the directory whose files need to be displayed.
*/
int display_all(unsigned short dir_inode_num)
{
	inode dir_inode;
	int i = 0, j = 0, k = 0, l = 0;
	unsigned short data_block_num;
	unsigned short data_block, data_block_1;
	char dir_name[14];
	inode infile_inode;
	unsigned short new_dir_inode_num;
	lseek(fd, 2 * BLOCK_SZ + (dir_inode_num * sizeof(inode)), SEEK_SET);
	read(fd, &dir_inode, sizeof(inode));
	if(!(dir_inode.flags & 0x4000))
	{
		printf("Not a valid directory \n");
		return -1;
	}
	if(!(dir_inode.flags & 0x1000))
        {
                for(i = 0; i < 8; i++)
                {
                        if(dir_inode.addr[i] == 0)
                        {
                                break;
                        }

                        data_block_num = dir_inode.addr[i];
                        //lseek(fd, BLOCK_SZ * data_block_num, SEEK_SET);
			if(i == 0)
                        {
                               // lseek(fd, 32, SEEK_CUR);      //first 32 bytes are for itself and parent dir
				j = 2;
                        }
			else
			{
				j = 0;
			}
                        for(; j < ((BLOCK_SZ/16)); j++)
                        {
				lseek(fd, BLOCK_SZ * data_block_num + (j * 16), SEEK_SET);
                                read(fd, &new_dir_inode_num, 2);
                                if(new_dir_inode_num != 0)
                                {
                                        read(fd, dir_name, 14);
					lseek(fd, 2 * BLOCK_SZ + (new_dir_inode_num * sizeof(inode)), SEEK_SET);
					read(fd, &infile_inode, sizeof(inode));
					if(infile_inode.flags & 0x4000)
					{
						printf("/%s \n",dir_name);
					}
					else
					{
						printf("%s \n",dir_name);
					}
                                }
                                else
                                {
                                        break;
                                }
                        }
                }
        }
	else
	{
		for(i = 0; i < 7 ; i++)
                {
                        if(dir_inode.addr[i] == 0)
                        {
                                break;
                        }
                        data_block_num = dir_inode.addr[i];
                        for(k = 0; k < BLOCK_SZ/2; k++)
                        {
                                lseek(fd, data_block_num * BLOCK_SZ + (k * 2), SEEK_SET);
                                read(fd, &data_block, 2);
                                if(data_block == 0)
                                {
                                        break;
                                }
                                lseek(fd, data_block * BLOCK_SZ, SEEK_SET);
                                j = 0;
                                if(i == 0 && k == 0)
                                {
                                        lseek(fd, 32, SEEK_CUR);      //first 32 bytes are for itself and parent dir
                                        j = 2;
                                }

				for(; j < ((BLOCK_SZ/16)); j++)
				{
					lseek(fd, BLOCK_SZ * data_block + (j * 16), SEEK_SET);
					read(fd, &new_dir_inode_num, 2);
					if(new_dir_inode_num != 0)
					{
						read(fd, dir_name, 14);
						lseek(fd, 2 * BLOCK_SZ + (new_dir_inode_num * sizeof(inode)), SEEK_SET);
						read(fd, &infile_inode, sizeof(inode));
						if(infile_inode.flags & 0x4000)
						{
							printf("/%s \n",dir_name);
						}
						else
						{
							printf("%s \n",dir_name);
						}
					}
					else
					{
						break;
					}
				}

                        }
                }
                if(i == 7)
                {
                        data_block_1 = dir_inode.addr[i];
                        if(data_block_1 == 0)
                        {
                                return 0;
                        }
                        for(l = 0; l < BLOCK_SZ/2; l++)
                        {
                                lseek(fd, data_block_1 * BLOCK_SZ + l * 2, SEEK_SET);
                                read(fd, &data_block_num,2);
                                if(data_block_num == 0)
                                {
                                        return 0;
                                }

                                for(k = 0; k < BLOCK_SZ/2; k++)
                                {
                                        lseek(fd, data_block_num * BLOCK_SZ + (k * 2), SEEK_SET);
                                        read(fd, &data_block, 2);
                                        if(data_block == 0)
                                        {
                                                return 0;
                                        }
                                        lseek(fd, data_block * BLOCK_SZ, SEEK_SET);
					for(j = 0; j < ((BLOCK_SZ/16)); j++)
					{
						lseek(fd, BLOCK_SZ * data_block + (j * 16), SEEK_SET);
						read(fd, &new_dir_inode_num, 2);
						if(new_dir_inode_num != 0)
						{
							read(fd, dir_name, 14);
							lseek(fd, 2 * BLOCK_SZ + (new_dir_inode_num * sizeof(inode)), SEEK_SET);
							read(fd, &infile_inode, sizeof(inode));
							if(infile_inode.flags & 0x4000)
							{
								printf("/%s \n",dir_name);
							}
							else
							{
								printf("%s \n",dir_name);
							}
						}
						else
						{
							break;
						}
					}
                                }
                        }
                }
	}
}

main(int argc, char *argv[])
{
	int i = 0, j = 0;
	char buffer[50], path[50];
	char cmd[10], total_blocks[10];
	char initfs_cmd[50];
	char ext_file[1000], int_file[1000], int_direct[1000];
	char directory[14];
	unsigned char dir_orfile[14];
        unsigned short dir_inode_num, file_inode_num, inode_num;
	char filename[14];
	int error;
	// TODO : initialize  incase initfs is already called  in previous execution	

	if(argc > 1)
	{
		if( access( argv[1], F_OK ) != -1 ) { /* File already exists */
			printf("File already exists and hence initializing metadata\n");
			Initialize_all_metadata(argv[1]);
			printf("Metadata initialization done. You can now directly run cpin, cpout or mkdir commands\n");
		}
		else
			printf("The (%s) file doesn't exist. Hence initialize a new filesystem (disk) using iitfs command\n", argv[1]);
	}

	while(1)
	{
label :		printf(">>");
                scanf("%s",cmd);

		if(strcmp(cmd, "initfs") == 0)
		{
			scanf("%s %d %d", path, &total_num_blocks, &total_inodes);
			initfs(path);
			//debug();
		}
		if(strcmp(cmd, "q") == 0)
		{
			lseek(fd, BLOCK_SZ,SEEK_SET);
			write(fd, &Super, sizeof(Super));
			break;
		}
		if(strcmp(cmd, "cpin") == 0)
		{
			scanf("%s %s",ext_file,int_file);
                        dir_inode_num = 0;   //initialize to root inode number
			i = 0;
			j = 0; 

			int fp = open(ext_file, O_RDWR);
			uint64_t length = lseek(fp, 0, SEEK_END)+1;
			close(fp);

			if(length > MAX_FILE_SIZE)
			{
				printf("File transfer of more than 32 MB is not supported in MyV6 Filesystem \n");
				goto label;
			}
			uint64_t size_available_in_disk = num_of_free_blocks_available() * 512;
			printf("File size = %lu\n", length);
			if(size_available_in_disk < length)
			{
				printf("Cannot copy this file (size %lu)\nDonnot have enough data blocks available\n", length);
				goto label;
			}

			while(int_file[i] != '\0')
			{
				if(int_file[i] != '/')
				{
					printf("Invalid file path to be copied \n");
					goto label;
				}

				i++;
				if(int_file[i] == '\0')
				{
					get_filename(ext_file,filename);
					if(filename == NULL)
					{
						goto label;
					}
					file_inode_num = find_dirInode(filename, dir_inode_num,0);
					if(file_inode_num != 0)
					{
						printf("file already exists in the current directory \n");
						goto label;
					}
					file_inode_num = create_file(ext_file, dir_inode_num);
					if(file_inode_num == 0)
					{
						printf("could not copy in the file \n");
						goto label;
					}

					entry_filename_dir(filename, dir_inode_num, file_inode_num);
					goto label;
				}
				for(j = 0;int_file[i] != '/'; i++,j++)
				{
					if(j > 13)
					{
						printf("filename is greater than 14 bytes \n");
						goto label;
					}
					if(int_file[i] == '\0')
					{
						break;
					}
					directory[j] = int_file[i];
				}
				directory[j] = '\0';

				if(int_file[i] == '\0')
				{
					file_inode_num = find_dirInode(directory, dir_inode_num,0);
                                        if(file_inode_num != 0)
                                        {
                                                printf("file already exists in the current directory \n");
                                                goto label;
                                        }

                                        file_inode_num = create_file(ext_file, dir_inode_num);
					if(file_inode_num == 0)
					{
						printf("could not copy in the file \n");
                                                goto label;
					}

                                        entry_filename_dir(directory, dir_inode_num, file_inode_num);
                                        goto label;
				}
				else
				{
					dir_inode_num = find_dirInode(directory, dir_inode_num,1);
					if(dir_inode_num == 0)
					{
						printf("%s directory is not present \n", directory);
						goto label;
					}
				}
			}
		}

		if(strcmp(cmd, "cpout") == 0)
		{
			scanf("%s %s",int_file,ext_file);
                        dir_inode_num = 0;   //initialize to root inode number
			i = 0; j = 0; 
                        while(int_file[i] != '\0')
                        {
                                if(int_file[i] != '/')
                                {
                                        printf("Invalid file path to be copied \n");
                                }

                                i++;
                                if(int_file[i] == '\0')
                                {
					printf("copy entire directory is not supported \n");
                                }

                                for(j = 0;int_file[i] != '/'; i++,j++)
                                {
					if(j > 13)
                                        {
                                                printf("filename is greater than 14 bytes \n");
						goto label;
                                        }
					if(int_file[i] == '\0')
					{
						break;
					}
                                        dir_orfile[j] = int_file[i];
                                }
                                dir_orfile[j] = '\0';

				if(int_file[i] == '\0')
                                {
                                        error = copy_file_ext(dir_orfile,ext_file, dir_inode_num);
					if(error == 0)
					{
						printf("could not copy the file properly \n");
						goto label;
					}
                                }
                                else
                                {
                                        dir_inode_num = find_dirInode(dir_orfile, dir_inode_num, 1);
                                        if(dir_inode_num == 0)
                                        {
                                                printf("%s directory is not present \n", dir_orfile);
                                        }
                                }
                        }
		}

		if(strcmp(cmd,"mkdir") == 0)
		{
			scanf("%s",int_direct);
			dir_inode_num = 0;   //initialize to root inode number
                        i = 0; j = 0;
                        while(int_direct[i] != '\0')
                        {
                                if(int_direct[i] != '/')
                                {
                                        printf("Invalid file path for directory to be created \n");
					goto label;  //TODO : change if cd is to be supported
                                }

                                i++;
                                if((int_direct[i] == '\0') && (i == 1))
                                {
                                        printf("The root directory is already created \n");
					goto label;
                                }
				else if(int_direct[i] == '\0')
				{
					inode_num = find_dirInode(dir_orfile, dir_inode_num, 1);
                                        if(inode_num != 0)
                                        {
                                                printf("Directory %s is already present \n",dir_orfile);
                                                goto label;
                                        }
                                        error = create_directory(dir_orfile, dir_inode_num);
					if(error == 0)
					{
						printf("Could not create the directory %s \n",dir_orfile);
					}
					else
					{
						printf("directory %s is created successfully \n",dir_orfile);
					}
                                        goto label;
				}
				

                                for(j = 0;int_direct[i] != '/'; i++,j++)
                                {
					if(j > 13)
                                        {
                                                printf("filename is greater than 14 bytes \n");
						goto label;
                                        }
                                        if(int_direct[i] == '\0')
                                        {
                                                break;
                                        }
                                        dir_orfile[j] = int_direct[i];
                                }
                                dir_orfile[j] = '\0';

                                if(int_direct[i] == '\0')
                                {
					inode_num = find_dirInode(dir_orfile, dir_inode_num, 1);
					if(inode_num != 0)
					{
						printf("Directory %s is already present \n",dir_orfile);
						goto label;
					}
                                        error = create_directory(dir_orfile, dir_inode_num);
					if(error == 0)
                                        {
                                                printf("Could not create the directory %s \n",dir_orfile);
                                        }
                                        else
                                        {
                                                printf("directory %s is created successfully \n",dir_orfile);
                                        }
					goto label;
                                }
                                else
                                {
					if(int_direct[i + 1] != '\0')
					{
                                        	dir_inode_num = find_dirInode(dir_orfile, dir_inode_num, 1);
                                        	if(dir_inode_num == 0)
                                        	{
							printf("%s directory is not present \n", dir_orfile);
                                        	}
					}
                                }
                        }
		}
		if(strcmp(cmd,"ls") == 0)
		{
			scanf("%s",int_direct);
                        dir_inode_num = 0;   //initialize to root inode number
                        i = 0; j = 0;
                        while(int_direct[i] != '\0')
                        {
                                if(int_direct[i] != '/')
                                {
                                        printf("Invalid file path for the files to be displayed \n");
                                        goto label; 
                                }

                                i++;
                                if(int_direct[i] == '\0')
                                {
                                        display_all(dir_inode_num);
					goto label;
                                }

                                for(j = 0;int_direct[i] != '/'; i++,j++)
                                {
                                        if(int_direct[i] == '\0')
                                        {
                                                break;
                                        }
                                        dir_orfile[j] = int_direct[i];
                                }
                                dir_orfile[j] = '\0';

				if(int_direct[i] == '\0')
                                {
                                        dir_inode_num = find_dirInode(dir_orfile, dir_inode_num, 1);
                                        if(dir_inode_num == 0)
                                        {
                                                printf("%s directory is not present \n", dir_orfile);
						goto label;
                                        }
					display_all(dir_inode_num);
                                }
                                else
                                {
                                        dir_inode_num = find_dirInode(dir_orfile, dir_inode_num, 1);
                                        if(dir_inode_num == 0)
                                        {
                                                printf("%s directory is not present \n", dir_orfile);
						goto label;
                                        }
                                }
                        }
		}
	}
	//debug();

}
