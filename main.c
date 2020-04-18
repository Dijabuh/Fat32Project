#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>
#include "str_func.h"

struct BPB {
	unsigned short bytes_per_sec;
	unsigned char sec_per_clus;
	unsigned short rsvd_sec_cnt;
	unsigned char num_fats;
	unsigned int tot_sec_32;
	unsigned int fat_sz_32;
	unsigned int root_clus;
}__attribute__((packed));

struct DIRENTRY {
	unsigned char dir_name[11];
	unsigned char dir_attr;
	unsigned short hi_fst_clus;
	unsigned short lo_fst_clus;
	unsigned int dir_file_size;
}__attribute__((packed));

void getBPB(struct BPB* bpb, int file) {
	unsigned char ch1, ch2, ch3, ch4;
	//bytes per sec
	lseek(file, 11, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	bpb->bytes_per_sec = (ch2 << 8) + ch1;

	//sec per cluster
	read(file, &ch1, 1);
	bpb->sec_per_clus = ch1;

	//reserved sec count
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	bpb->rsvd_sec_cnt = (ch2 << 8) + ch1;

	//num fats
	read(file, &ch1, 1);
	bpb->num_fats = ch1;

	//tot sec 32
	lseek(file, 32, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	read(file, &ch3, 1);
	read(file, &ch4, 1);
	bpb->tot_sec_32 = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;

	//fat size 32
	lseek(file, 36, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	read(file, &ch3, 1);
	read(file, &ch4, 1);
	bpb->fat_sz_32 = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;

	//root cluster
	lseek(file, 44, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	read(file, &ch3, 1);
	read(file, &ch4, 1);
	bpb->root_clus = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;
}

// 0 if no entry, 1 if file exists
int file_exists(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster);

void info_cmd(struct BPB* bpb) {
	printf("Bytes Per Sector: %hu\n", bpb->bytes_per_sec);
	printf("Sectors Per Cluster: %hhu\n", bpb->sec_per_clus);
	printf("Reserved Sector Count: %hu\n", bpb->rsvd_sec_cnt);
	printf("Number of Fats: %hhu\n", bpb->num_fats);
	printf("Total Sectors: %u\n", bpb->tot_sec_32);
	printf("Fat Size: %u\n", bpb->fat_sz_32);
	printf("Root Cluster: %u\n", bpb->root_clus);
}

//returns the offset in bytes of the data location of the given cluster
unsigned int get_data_location(unsigned int cluster, struct BPB* bpb) {
	return bpb->bytes_per_sec * (((cluster - 2) * bpb->sec_per_clus) + (bpb->rsvd_sec_cnt + (bpb->num_fats * bpb->fat_sz_32))); 
}

void get_dir_entry(struct DIRENTRY* de, int file, unsigned int location) {
	lseek(file, location, SEEK_SET);
	read(file, &(de->dir_name), 11);
	read(file, &(de->dir_attr), 1);
	lseek(file, location+20, SEEK_SET);
	unsigned char ch1, ch2, ch3, ch4;
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	de->hi_fst_clus = (ch2 << 8) + ch1;
	lseek(file, location+26, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	de->lo_fst_clus = (ch2 << 8) + ch1;
	unsigned int DIR_FileSize;
	lseek(file, location+28, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	read(file, &ch3, 1);
	read(file, &ch4, 1);
	de->dir_file_size = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;

	//remove trailing spaces from name
	for(int i = 10; i >= 0; i--) {
		if(de->dir_name[i] == 0x20) {
			de->dir_name[i] = '\0';
		}
		else {
			break;
		}
	}
}

//returns the cluster following the given cluster
//returns 0 if the given cluster is the last one
unsigned int get_next_cluster(unsigned int cluster, struct BPB* bpb, int file) {
	unsigned int byte_offset = bpb->rsvd_sec_cnt * bpb->bytes_per_sec + cluster * 4;
	lseek(file, byte_offset, SEEK_SET);
	unsigned char ch1, ch2, ch3, ch4;
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	read(file, &ch3, 1);
	read(file, &ch4, 1);
	unsigned int new_clus = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;

	if((new_clus >= 0x0FFFFFF8 && new_clus <= 0x0FFFFFFE) || new_clus == 0xFFFFFFFF) {
		return 0;
	}

	return new_clus;

}

void ls_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster) {
	unsigned int clus;
	if(cmd->numParts == 1) {
		//ls of currect directory
		clus = start_cluster;
	}
	else if(cmd->numParts == 2) {
		//run ls on dir name given if it exists
		//loop through dir entry for given start cluster
		//if name of one of them matches given name, set its lo/hi first cluster to clus
		unsigned int temp_clus = start_cluster;
		int quit = 0;
		while(temp_clus > 0) {
			//get data location from cluster number
			unsigned int location = get_data_location(temp_clus, bpb);
			location -= 32;

			//loop through all dir entries here
			for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
				location += 32;
	
				//get dir entry
				struct DIRENTRY de;
				get_dir_entry(&de, file, location);
	
				if(strcmp(de.dir_name, cmd->parts[1]) == 0) {
					if(de.dir_attr != 0x10){
						printf("%s is not a directory\n", cmd->parts[1]);
						return;
					}
					clus = (de.hi_fst_clus << 16) + de.lo_fst_clus;
					quit = 1;
					break;
				}
			}
			if(quit) {
				break;
			}
			//get next cluster
			temp_clus = get_next_cluster(clus, bpb, file);
		}
		if(!quit) {
			printf("Directory %s does not exists\n", cmd->parts[1]);
			return;
		}
	}
	else {
		printf("Wrong number of arguements for ls command\n");
	}

	while(clus > 0) {
		//get data location from cluster number
		unsigned int location = get_data_location(clus, bpb);
		location -= 32;

		//loop through all dir entries here
		for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
			location += 32;

			//get dir entry
			struct DIRENTRY de;
			get_dir_entry(&de, file, location);

			if(de.dir_name[0] == 0x00) {
				return;
			}

			if(de.dir_attr == 0x0F) {
				//long name entry, do nothing
			}
			else if(de.dir_name[0] == 0xE5) {
				//empty entry, do nothing
			}
			else {
				printf("%s\n", de.dir_name);
			}
		}

		//get next cluster
		clus = get_next_cluster(clus, bpb, file);
	}
}

void size_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster) {
	unsigned int clus = start_cluster;

	if(cmd->numParts == 2) {
		//run size on dir name given if it exists
		//loop through dir entry for given start cluster
		//if name of one of them matches given name, print its size
		int quit = 0;
		while(clus > 0) {
			//get data location from cluster number
			unsigned int location = get_data_location(clus, bpb);
			location -= 32;

			//loop through all dir entries here
			for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
				location += 32;
	
				//get dir entry
				struct DIRENTRY de;
				get_dir_entry(&de, file, location);
	
				
				if(de.dir_name[0] == 0x00) {
					printf("File %s does not exists\n", cmd->parts[1]);
					return;
				}

				if(de.dir_attr == 0x0F) {
					//long name entry, do nothing
				}
				else if(de.dir_name[0] == 0xE5) {
					//empty entry, do nothing
				}
				else if(strcmp(de.dir_name, cmd->parts[1]) == 0) {
					if(de.dir_attr != 0x20){
						printf("%s is not a file\n", cmd->parts[1]);
						return;
					}

					printf("File %s is %d bytes in size.\n",
						de.dir_name, de.dir_file_size);
					quit = 1;
					break;
				}
			}
			if(quit) {
				break;
			}
			//get next cluster
			clus = get_next_cluster(clus, bpb, file);
		}
		if(!quit) {
			printf("File %s does not exists\n", cmd->parts[1]);
			return;
		}
	}
	else {
		printf("Wrong number of arguements for size command\n");
	}
}

unsigned int cd_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster) {
	unsigned int clus = star_cluster;

	if(cmd->numParts == 2) {
		//run cd on dir name given if it exists
		//loop through dir entry for given start cluster
		//if name of one of them matches given name, return its start cluster
		while(clus > 0) {
			//get data location from cluster number
			unsigned int location = get_data_location(clus, bpb);
			location -= 32;

			//loop through all dir entries here
			for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
				location += 32;
	
				//get dir entry
				struct DIRENTRY de;
				get_dir_entry(&de, file, location);
	
				if(de.dir_name[0] == 0x00) {
					printf("Directory %s does not exists\n", cmd->parts[1]);
					return 0;
 				}

				if(de.dir_attr == 0x0F) {
					//long name entry, do nothing
				}
				else if(de.dir_name[0] == 0xE5) {
					//empty entry, do nothing
				}
				else if(strcmp(de.dir_name, cmd->parts[1]) == 0) {
					if(de.dir_attr != 0x10){
						printf("%s is not a directory\n", cmd->parts[1]);
						return 0;
					}
					if((de.hi_fst_clus << 16) + de.lo_fst_clus == 0) {
						return 2;
					}
					return ((de.hi_fst_clus << 16) + de.lo_fst_clus);
				}
			}
			//get next cluster
			clus = get_next_cluster(clus, bpb, file);
		}
		printf("Directory %s does not exists\n", cmd->parts[1]);
		return 0;
	}
	else {
		printf("Wrong number of arguements for cd command\n");
		return 0;
	}
}

int main(int argc, char** argv) {
	struct BPB bpb;
	char* buf;
	buf = (char*) malloc(sizeof(char) * 101);
	
	if(argc != 2) {
		printf("Wrong number of arguments\n");
		return 0;
	}

	int file = open(argv[1], O_RDWR);

	getBPB(&bpb, file);

	unsigned int start_cluster = 2;

	while(1) {
		printf("$ ");
		fgets(buf, 100, stdin);
		strtok(buf, "\n");

		pathparts cmd;
		splitString(&cmd, buf, " ");

		if(strcmp(cmd.parts[0], "exit") == 0) {
			return 0;
		}
		else if(strcmp(cmd.parts[0], "info") == 0) {
			info_cmd(&bpb);
		}
		else if(strcmp(cmd.parts[0], "ls") == 0) {
			ls_cmd(&bpb, file, &cmd, start_cluster);
		}
		else if(strcmp(cmd.parts[0], "size") == 0) {
			size_cmd(&bpb, file, &cmd, start_cluster);
		}
		else if(strcmp(cmd.parts[0], "cd") == 0) {
			int ret = cd_cmd(&bpb, file, &cmd, start_cluster);
			if(ret > 0) {
				start_cluster = ret;
			}
		}
		else if(strcmp(cmd.parts[0], "creat") == 0) {
			int exists = file_exists(&bpb, file, &cmd, start_cluster);
			if(exists == 1)
				printf("File %s already exists\n", cmd.parts[1]);
			else if(exists == -1)
				printf("Wrong number of arguements for creat command\n");
			else{
				// run creat command
			}
		}
	}

	return 0;
}

int file_exists(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster) {
	
	if(cmd->numParts != 2) { return -1; }

	// run on dir name given if it exists
	// loop through dir entry for given start cluster
	// if name of one of them matches given name, returns error
	unsigned int clus = start_cluster;
	while(clus > 0) {
		//get data location from cluster number
		unsigned int location = get_data_location(clus, bpb);
		location -= 32;

		//loop through all dir entries here
		for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
			location += 32;

			// get dir entry
			struct DIRENTRY de;
			get_dir_entry(&de, file, location);

			if(strcmp(de.dir_name, cmd->parts[1]) == 0) {
				return 1;
			}
		}
		//get next cluster
		clus = get_next_cluster(clus, bpb, file);
	}
	return 0;
}
