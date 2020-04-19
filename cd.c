#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

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
