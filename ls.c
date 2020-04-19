#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

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
