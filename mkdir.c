#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

void mkdir_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster) {
	if(cmd->numParts != 2) {
		printf("Wrong number of arguements for mkdir command\n");
		return;
	}

	//first check if given dirname already exists
	unsigned int clus = star_cluster;

	while(clus > 0) {
		//get data location from cluster number
		unsigned int location = get_data_location(clus, bpb);
		location -= 32;

		int quit = 0;
		//loop through all dir entries here
		for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
			location += 32;

			//get dir entry
			struct DIRENTRY de;
			get_dir_entry(&de, file, location);
	
			if(de.dir_name[0] == 0x00) {
				quit = 1;
				break;
			}

			if(de.dir_attr == 0x0F) {
				//long name entry, do nothing
			}
			else if(de.dir_name[0] == 0xE5) {
				//empty entry, do nothing
			}
			else if(strcmp(de.dir_name, cmd->parts[1]) == 0) {
				printf("%s already exists\n", cmd->parts[1]);
				return;
			}
		}
		//get next cluster
		clus = get_next_cluster(clus, bpb, file);
		if(quit) {
			break;
		}
	}

	//now, loop through all dir entries in starting cluster
	//if we encounter an empty one, add a new direntry there
	clus = star_cluster;
	unsigned int last_cluster;
	while(clus > 0) {
		//get data location from cluster number
		unsigned int location = get_data_location(clus, bpb);
		location -= 32;

		int quit = 0;
		//loop through all dir entries here
		for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
			location += 32;

			//get dir entry
			struct DIRENTRY de;
			get_dir_entry(&de, file, location);
	
			if(de.dir_name[0] == 0x00 || de.dir_name[0] == 0xE5) {
				//add dir entry here
				//create a new cluster for the new directory
				unsigned int new_dir_clus = get_next_empty_cluster(bpb, file);
				//set its next cluster to 0xFFFFFFFF
				set_next_cluster(bpb, file, new_dir_clus, 0xFFFFFFFF);

				create_new_dir(bpb, file, cmd, location, clus, new_dir_clus);
				//make next entry empty
				//char tmp;
				//tmp = 0xE5;
				//write(file, &tmp, 1);
				return;
			}
			if(de.dir_attr == 0x0F) {
				//long name entry, do nothing
			}
		}
		//get next cluster
		last_cluster = clus;
		clus = get_next_cluster(clus, bpb, file);
	}
	//if we are here, all dir entries are taken
	//need to allocate new cluster for current directory
	unsigned next_dir_clus = get_next_empty_cluster(bpb, file);
	//set the next cluster for the last one we were at to the newly allocated one
	set_next_cluster(bpb, file, last_cluster, next_dir_clus);
	//set the next cluster for the newly allocated one to 0xFFFFFFFF
	set_next_cluster(bpb, file, next_dir_clus, 0xFFFFFFFF);

	//create a new cluster for the new directory
	unsigned int new_dir_clus = get_next_empty_cluster(bpb, file);
	//set its next cluster to 0xFFFFFFFF
	set_next_cluster(bpb, file, new_dir_clus, 0xFFFFFFFF);

	//put in data for the new dir entry 
	//get data location
	unsigned int location = get_data_location(next_dir_clus, bpb);
	create_new_dir(bpb, file, cmd, location, next_dir_clus, new_dir_clus);
	//make next entry empty
	lseek(file, location + 32, SEEK_SET);
	char tmp;
	tmp = 0x00;
	write(file, &tmp, 1);
}
