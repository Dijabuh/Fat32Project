#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

void rm_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster) {
	//steps
	//check if file exists
	//if not, error
	//check if file is directory
	//if it is,  error
	//set first character of dir entry name to 0xE5
	//go to first cluster in FAT
	//copy its next cluster
	//set it to 0x00000000
	//go to next cluster and repeat
	//stop when next cluster is 0
	if(cmd->numParts != 2) {
		printf("Wrong number of arguements for rm command\n");
		return;
	}

	//check if file exists and is a file
	unsigned int clus = star_cluster;
	struct DIRENTRY de;
	int quit= 0;
	while(clus > 0) {
		//get data location from cluster number
		unsigned int location = get_data_location(clus, bpb);
		location -= 32;

		quit = 0;
		//loop through all dir entries here
		for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
			location += 32;

			//get dir entry
			get_dir_entry(&de, file, location);
	
			if(de.dir_name[0] == 0x00) {
				printf("%s does not exists\n", cmd->parts[1]);
				return;
			}

			if(de.dir_attr == 0x0F) {
				//long name entry, do nothing
			}
			else if(de.dir_name[0] == 0xE5) {
				//empty entry, do nothing
			}
			else if(strcmp(de.dir_name, cmd->parts[1]) == 0) {
				//file exists, make sure it is file, not directory
				if(de.dir_attr == 0x20) {
					//set first char of direntry to 0xE5
					lseek(file, location, SEEK_SET);
					unsigned char tmp = 0xE5;
					write(file, &tmp, 1);
					quit = 1;
					break;
				}
				printf("%s is not a file\n", cmd->parts[1]);
				return;
			}
		}
		//get next cluster
		clus = get_next_cluster(clus, bpb, file);
		if(quit) {
			break;
		}
	}
	if(!quit) {
		printf("%s does not exists\n", cmd->parts[1]);
		return;
	}

	//get first cluster of file
	clus = (de.hi_fst_clus << 16) + de.lo_fst_clus;

	while(clus > 0) {
		//loop through all clusters in this line
		unsigned int temp = get_next_cluster(clus, bpb, file);

		//set current clusters next cluster to 0x00000000
		set_next_cluster(bpb, file, clus, 0x00000000);

		clus = temp;
	}
}
