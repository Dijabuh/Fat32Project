#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

void cp_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster) {
	//steps
	//check if filename is a file that exists
	//if not, error
	//check if to exists and is a directory
	//if to exists and is a file, error
	//if to doesnt exists, cp file to current directory with new name
	//follow the cluster trail for file
	//for each cluster, allocate a new one for the copied file and copy all 512 bytes into it
	//make sure they point at eachother
	//create a new direntry in the correct new directory
	if(cmd->numParts != 3) {
		printf("Wrong number of arguements for cp command\n");
		return;
	}

	//check if file exists and is a file
	unsigned int clus = star_cluster;
	struct DIRENTRY file_dir_entry;
	struct DIRENTRY to_dir_entry;

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
			get_dir_entry(&file_dir_entry, file, location);
	
			if(file_dir_entry.dir_name[0] == 0x00) {
				printf("%s does not exists\n", cmd->parts[1]);
				return;
			}

			if(file_dir_entry.dir_attr == 0x0F) {
				//long name entry, do nothing
			}
			else if(file_dir_entry.dir_name[0] == 0xE5) {
				//empty entry, do nothing
			}
			else if(strcmp(file_dir_entry.dir_name, cmd->parts[1]) == 0) {
				//file exists, make sure it is file, not directory
				if(file_dir_entry.dir_attr == 0x20) {
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

	//check if to exists
	//
	quit = 0;
	int to_exists = 0;
	while(clus > 0) {
		//get data location from cluster number
		unsigned int location = get_data_location(clus, bpb);
		location -= 32;

		quit = 0;
		//loop through all dir entries here
		for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
			location += 32;

			//get dir entry
			get_dir_entry(&to_dir_entry, file, location);
	
			if(to_dir_entry.dir_name[0] == 0x00) {
				quit = 1;
				break;
			}

			if(to_dir_entry.dir_attr == 0x0F) {
				//long name entry, do nothing
			}
			else if(to_dir_entry.dir_name[0] == 0xE5) {
				//empty entry, do nothing
			}
			else if(strcmp(to_dir_entry.dir_name, cmd->parts[2]) == 0) {
				//file exists, make sure it is directory, not file
				if(to_dir_entry.dir_attr == 0x10) {
					quit = 1;
					to_exists = 1;
					break;
				}
				printf("%s is not a directory\n", cmd->parts[2]);
				return;
			}
		}
		//get next cluster
		clus = get_next_cluster(clus, bpb, file);
		if(quit) {
			break;
		}
	}

	//get starting cluster for file
	clus = (file_dir_entry.hi_fst_clus << 16) + file_dir_entry.lo_fst_clus;
	unsigned int last_cluster;
	unsigned int new_first_cluster;
	unsigned int cur_cluster;

	int first = 1;
	while(clus > 0) {
		//loop through all clusters in this line
		//allocate a new cluster
		if(first) {
			//first time through, just allocate 1 cluster
			first = 0;
			new_first_cluster = get_next_empty_cluster(bpb, file);
			cur_cluster = new_first_cluster;
		}
		else {
			//allocate 1 new cluster, set last cluster to point at it
			cur_cluster = get_next_empty_cluster(bpb, file);
			set_next_cluster(bpb, file, last_cluster, cur_cluster);
		}

		//copy data from files current cluster to new files current cluster
		//location of source 
		unsigned int location = get_data_location(clus, bpb);
		lseek(file, location, SEEK_SET);
		//copy all 512 bytes
		char tmp[512];
		read(file, &tmp, 512);
		//location of destination
		location = get_data_location(cur_cluster, bpb);
		write(file, &tmp, 512);
		
		//set last_cluster to cur_cluster
		last_cluster = cur_cluster;

		//get next cluster in file
		clus = get_next_cluster(clus, bpb, file);
	}
	//set destination of last cluster to 0xFFFFFFFF
	set_next_cluster(bpb, file, last_cluster, 0xFFFFFFFF);

	//create new directory entry struct
	struct DIRENTRY new_entry;
	if(to_exists) {
		strcpy(new_entry.dir_name, file_dir_entry.dir_name);
	}
	else {
		strcpy(new_entry.dir_name, cmd->parts[2]);
	}
	new_entry.dir_attr = file_dir_entry.dir_attr;
	new_entry.hi_fst_clus = new_first_cluster >> 16;
	new_entry.lo_fst_clus = new_first_cluster;
	new_entry.dir_file_size = file_dir_entry.dir_file_size;

	if(to_exists) {
		clus = (to_dir_entry.hi_fst_clus << 16) + to_dir_entry.lo_fst_clus;
	}
	else {
		clus = star_cluster;
	}

	//go through the cluster until we find an empty direntry
	
	char tmp;
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
				//add dir  entry for from here
				//set name
				lseek(file, location, SEEK_SET);
				for(int i = 0; i < 8; i++) {
					if(i < strlen(new_entry.dir_name)) {
						write(file, &(new_entry.dir_name[i]), 1);
					}
					else {
						write(file, " ", 1);
					}
				}
				write(file, "   ", 3);
				//write dir attr
				tmp = new_entry.dir_attr;
				write(file, &tmp, 1);
				//write 0s to unused bits
				write(file, "\0\0\0\0\0\0\0\0", 8);
				//write hi bits of cluster
				tmp = new_entry.hi_fst_clus & 0xFF;
				write(file, &tmp, 1);
				tmp = (new_entry.hi_fst_clus >> 8) & 0xFF;
				write(file, &tmp, 1);
				//write 0s to unused bits
				write(file, "\0\0\0\0", 4);
				//write lo bits of cluster
				tmp = new_entry.lo_fst_clus & 0xFF;
				write(file, &tmp, 1);
				tmp = (new_entry.lo_fst_clus >> 8) & 0xFF;
				write(file, &tmp, 1);
				//write file size
				tmp = new_entry.dir_file_size & 0xFF;
				write(file, &tmp, 1);
				tmp = (new_entry.dir_file_size >> 8) & 0xFF;
				write(file, &tmp, 1);
				tmp = (new_entry.dir_file_size >> 16) & 0xFF;
				write(file, &tmp, 1);
				tmp = (new_entry.dir_file_size >> 24) & 0xFF;
				write(file, &tmp, 1);

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

	//add in new dir_entry for from
	unsigned int location = get_data_location(next_dir_clus, bpb);
	//set name
	lseek(file, location, SEEK_SET);
	for(int i = 0; i < 8; i++) {
		if(i < strlen(new_entry.dir_name)) {
			write(file, &(new_entry.dir_name[i]), 1);
		}
		else {
			write(file, " ", 1);
		}
	}
	write(file, "   ", 3);
	//write dir attr
	tmp = new_entry.dir_attr;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0\0\0\0\0", 8);
	//write hi bits of cluster
	tmp = new_entry.hi_fst_clus & 0xFF;
	write(file, &tmp, 1);
	tmp = (new_entry.hi_fst_clus >> 8) & 0xFF;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0", 4);
	//write lo bits of cluster
	tmp = new_entry.lo_fst_clus & 0xFF;
	write(file, &tmp, 1);
	tmp = (new_entry.lo_fst_clus >> 8) & 0xFF;
	write(file, &tmp, 1);
	//write file size
	tmp = new_entry.dir_file_size & 0xFF;
	write(file, &tmp, 1);
	tmp = (new_entry.dir_file_size << 8) & 0xFF;
	write(file, &tmp, 1);
	tmp = (new_entry.dir_file_size << 16) & 0xFF;
	write(file, &tmp, 1);
	tmp = (new_entry.dir_file_size << 24) & 0xFF;
	write(file, &tmp, 1);
	
	//make next entry empty
	lseek(file, location + 32, SEEK_SET);
	tmp = 0x00;
	write(file, &tmp, 1);
}


