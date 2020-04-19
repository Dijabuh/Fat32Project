#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

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
