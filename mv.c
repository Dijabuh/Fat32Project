#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

void mv_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster) {
	if(cmd->numParts != 3) {
		printf("Wrong number of arguements for mv command\n");
		return;
	}

	if(strcmp(cmd->parts[1], ".") == 0 || strcmp(cmd->parts[1], "..") == 0) {
		printf("Cant move directory %s\n", cmd->parts[1]);
		return;
	}

	unsigned int clus = star_cluster;
	struct DIRENTRY from_dir_entry;
	struct DIRENTRY to_dir_entry;
	unsigned int from_entry_loc;
	unsigned int to_entry_loc;
	int quit = 0;

	//check if from exists
	while(clus > 0) {
		//get data location from cluster number
		unsigned int location = get_data_location(clus, bpb);
		location -= 32;

		quit = 0;
		//loop through all dir entries here
		for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
			location += 32;

			//get dir entry
			from_entry_loc = location;
			get_dir_entry(&from_dir_entry, file, location);
	
			if(from_dir_entry.dir_name[0] == 0x00) {
				printf("%s does not exists\n", cmd->parts[1]);
				return;
			}

			if(from_dir_entry.dir_attr == 0x0F) {
				//long name entry, do nothing
			}
			else if(from_dir_entry.dir_name[0] == 0xE5) {
				//empty entry, do nothing
			}
			else if(strcmp(from_dir_entry.dir_name, cmd->parts[1]) == 0) {
				quit = 1;
				break;
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
	quit = 0;
	int to_exists = 0;
	clus = star_cluster;
	while(clus > 0) {
		//get data location from cluster number
		unsigned int location = get_data_location(clus, bpb);
		location -= 32;

		int quit = 0;
		//loop through all dir entries here
		for(int i = 0; i < bpb->bytes_per_sec/32; i++) {
			location += 32;

			//get dir entry
			to_entry_loc = location;
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
				//if to exists and is not a directory, error
				if(to_dir_entry.dir_attr != 0x10) {
					if(from_dir_entry.dir_attr == 0x10) {
						printf("Cannot move directory: invalid  destination argument\n");
						return;
					}
					printf("The name is already being used by a file\n");
					return;
				}

				to_exists = 1;
				quit = 1;
				break;
			}
		}
		//get next cluster
		clus = get_next_cluster(clus, bpb, file);
		if(quit) {
			break;
		}
	}
	
	//if to dir exists, move from dir_entry to to directory
	if(to_exists) {
		//set first byte of from's dir entry to 0xE5
		lseek(file, from_entry_loc, SEEK_SET);
		char tmp = 0xE5;
		write(file, &tmp, 1);

		//find an empty direntry in to's directory

		//get cluster for to's directory
		clus = (to_dir_entry.hi_fst_clus << 16) + to_dir_entry.lo_fst_clus;
		unsigned int first_cluster = clus;
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
					//add dir  entry for from here
					//set name
					lseek(file, location, SEEK_SET);
					for(int i = 0; i < 8; i++) {
						if(i < strlen(from_dir_entry.dir_name)) {
							write(file, &(from_dir_entry.dir_name[i]), 1);
						}
						else {
							write(file, " ", 1);
						}
					}
					write(file, "   ", 3);
					//write dir attr
					tmp = from_dir_entry.dir_attr;
					write(file, &tmp, 1);
					//write 0s to unused bits
					write(file, "\0\0\0\0\0\0\0\0", 8);
					//write hi bits of cluster
					tmp = from_dir_entry.hi_fst_clus & 0xFF;
					write(file, &tmp, 1);
					tmp = (from_dir_entry.hi_fst_clus >> 8) & 0xFF;
					write(file, &tmp, 1);
					//write 0s to unused bits
					write(file, "\0\0\0\0", 4);
					//write lo bits of cluster
					tmp = from_dir_entry.lo_fst_clus & 0xFF;
					write(file, &tmp, 1);
					tmp = (from_dir_entry.lo_fst_clus >> 8) & 0xFF;
					write(file, &tmp, 1);
					//write file size
					tmp = from_dir_entry.dir_file_size & 0xFF;
					write(file, &tmp, 1);
					tmp = (from_dir_entry.dir_file_size >> 8) & 0xFF;
					write(file, &tmp, 1);
					tmp = (from_dir_entry.dir_file_size >> 16) & 0xFF;
					write(file, &tmp, 1);
					tmp = (from_dir_entry.dir_file_size >> 24) & 0xFF;
					write(file, &tmp, 1);

					//if from is a directory, update its .. entry to point to its new parent directory
					if(from_dir_entry.dir_attr == 0x10) {
						//get its location
						clus = (from_dir_entry.hi_fst_clus << 16) + from_dir_entry.lo_fst_clus;
						location = get_data_location(clus, bpb);
						//go to .. entry
						location += 32;
						//go to hi_fst_clus offset
						location += 20;
						lseek(file, location, SEEK_SET);
						//write hi_fst_clus bytes
						tmp = to_dir_entry.hi_fst_clus & 0xFF;
						write(file, &tmp, 1);
						tmp = (to_dir_entry.hi_fst_clus >> 8) & 0xFF;
						write(file, &tmp, 1);
						//go to lo_fst_clus offset
						location += 6;
						lseek(file, location, SEEK_SET);
						//write lo_fst_clus bytes
						tmp = to_dir_entry.lo_fst_clus & 0xFF;
						write(file, &tmp, 1);
						tmp = (to_dir_entry.lo_fst_clus >> 8) & 0xFF;
						write(file, &tmp, 1);
					}
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
			if(i < strlen(from_dir_entry.dir_name)) {
				write(file, &(from_dir_entry.dir_name[i]), 1);
			}
			else {
				write(file, " ", 1);
			}
		}
		write(file, "   ", 3);
		//write dir attr
		tmp = from_dir_entry.dir_attr;
		write(file, &tmp, 1);
		//write 0s to unused bits
		write(file, "\0\0\0\0\0\0\0\0", 8);
		//write hi bits of cluster
		tmp = from_dir_entry.hi_fst_clus & 0xFF;
		write(file, &tmp, 1);
		tmp = (from_dir_entry.hi_fst_clus >> 8) & 0xFF;
		write(file, &tmp, 1);
		//write 0s to unused bits
		write(file, "\0\0\0\0", 4);
		//write lo bits of cluster
		tmp = from_dir_entry.lo_fst_clus & 0xFF;
		write(file, &tmp, 1);
		tmp = (from_dir_entry.lo_fst_clus >> 8) & 0xFF;
		write(file, &tmp, 1);
		//write file size
		tmp = from_dir_entry.dir_file_size & 0xFF;
		write(file, &tmp, 1);
		tmp = (from_dir_entry.dir_file_size << 8) & 0xFF;
		write(file, &tmp, 1);
		tmp = (from_dir_entry.dir_file_size << 16) & 0xFF;
		write(file, &tmp, 1);
		tmp = (from_dir_entry.dir_file_size << 24) & 0xFF;
		write(file, &tmp, 1);
		//if from is a directory, update its .. entry to point to its new parent directory
		if(from_dir_entry.dir_attr == 0x10) {
			//get its location
			clus = (from_dir_entry.hi_fst_clus << 16) + from_dir_entry.lo_fst_clus;
			location = get_data_location(clus, bpb);
			//go to .. entry
			location += 32;
			//go to hi_fst_clus offset
			location += 20;
			lseek(file, location, SEEK_SET);
			//write hi_fst_clus bytes
			tmp = to_dir_entry.hi_fst_clus & 0xFF;
			write(file, &tmp, 1);
			tmp = (to_dir_entry.hi_fst_clus >> 8) & 0xFF;
			write(file, &tmp, 1);
			//go to lo_fst_clus offset
			location += 4;
			lseek(file, location, SEEK_SET);
			//write lo_fst_clus bytes
			tmp = to_dir_entry.lo_fst_clus & 0xFF;
			write(file, &tmp, 1);
			tmp = (to_dir_entry.lo_fst_clus >> 8) & 0xFF;
			write(file, &tmp, 1);
		}
		
		//make next entry empty
		lseek(file, location + 32, SEEK_SET);
		tmp = 0x00;
		write(file, &tmp, 1);
		return;
	}

				printf("here\n");
	//if to does not exists, change the name of the dir entry for from
	lseek(file, from_entry_loc, SEEK_SET);
	for(int i = 0; i < 8; i++) {
		if(i < strlen(cmd->parts[2])) {
			write(file, &(cmd->parts[2][i]), 1);
		}
		else {
			write(file, " ", 1);
		}
	}
}
