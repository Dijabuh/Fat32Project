#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

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

//	if((new_clus >= 0x0FFFFFF8 && new_clus <= 0x0FFFFFFE) || new_clus == 0xFFFFFFFF) {
//		return 0;
//	}
	
	//trying something new
	if(new_clus > bpb->tot_sec_32) {
		return 0;
	}

	return new_clus;

}

//returns the cluster number of the first empty(0x00000000) cluster
unsigned int get_next_empty_cluster(struct BPB* bpb, int file) {
	//go to start of FAT section
	lseek(file, bpb->rsvd_sec_cnt * bpb->bytes_per_sec, SEEK_SET);
	for(unsigned int i = 0; i < bpb->num_fats * bpb->fat_sz_32; i++) {
		//get in 4 bytes
		unsigned char ch1, ch2, ch3, ch4;
		read(file, &ch1, 1);
		read(file, &ch2, 1);
		read(file, &ch3, 1);
		read(file, &ch4, 1);

		unsigned int clus_val = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;

		if(clus_val == 0x00000000) {
			//is an empty cluster, return the cluster number i
			return i;
		}	
	}
	return 0;
}

//sets the value of the next cluster for the currect cluster
void set_next_cluster(struct BPB* bpb, int file, unsigned int cur_cluster, unsigned int next_cluster) {
	lseek(file, (bpb->rsvd_sec_cnt * bpb->bytes_per_sec) + (cur_cluster * 4), SEEK_SET);
	//write the value of next_cluster
	unsigned char ch1, ch2, ch3, ch4;
	ch1 = (next_cluster & 0xFF);
	ch2 = ((next_cluster >> 8) & 0xFF);
	ch3 = ((next_cluster >> 16) & 0xFF);
	ch4 = ((next_cluster >> 24) & 0xFF);
	write(file, &ch1, 1);
	write(file, &ch2, 1);
	write(file, &ch3, 1);
	write(file, &ch4, 1);
}

void create_new_dir(struct BPB* bpb, int file, pathparts* cmd, unsigned int dir_entry_location, unsigned int parent_clus, unsigned int dir_clus) {

	//create a new cluster for the new directory
	unsigned int new_dir_clus = dir_clus;
	unsigned int clus = parent_clus;
	unsigned int location = dir_entry_location;

	//put in data for the new dir entry 
	//get data location
	lseek(file, location, SEEK_SET);
	//set name
	for(int i = 0; i < 8; i++) {
		if(i < strlen(cmd->parts[1])) {
			write(file, &(cmd->parts[1][i]), 1);
		}
		else {
			write(file, " ", 1);
		}
	}
	write(file, "   ", 3);
	//write dir attr
	char tmp = 0x10;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0\0\0\0\0", 8);
	//write hi bits of cluster
	tmp = (new_dir_clus >> 16) & 0xFF;
	write(file, &tmp, 1);
	tmp = (new_dir_clus >> 24) & 0xFF;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0", 4);
	//write lo bits of cluster
	tmp = new_dir_clus & 0xFF;
	write(file, &tmp, 1);
	tmp = (new_dir_clus >> 8) & 0xFF;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0", 4);

	//in data location for new directory, put in dir entries for . and ..
	location = get_data_location(new_dir_clus, bpb);
	lseek(file, location, SEEK_SET);
	//set name
	write(file, ".          ", 11);
	//write dir attr
	tmp = 0x10;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0\0\0\0\0", 8);
	//write hi bits of cluster
	tmp = (new_dir_clus >> 16) & 0xFF;
	write(file, &tmp, 1);
	tmp = (new_dir_clus >> 24) & 0xFF;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0", 4);
	//write lo bits of cluster
	tmp = new_dir_clus & 0xFF;
	write(file, &tmp, 1);
	tmp = (new_dir_clus >> 8) & 0xFF;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0", 4);
	//set name
	write(file, "..         ", 11);
	//write dir attr
	tmp = 0x10;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0\0\0\0\0", 8);
	//write hi bits of cluster
	tmp = (clus >> 16) & 0xFF;
	write(file, &tmp, 1);
	tmp = (clus >> 24) & 0xFF;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0", 4);
	//write lo bits of cluster
	tmp = clus & 0xFF;
	write(file, &tmp, 1);
	tmp = (clus >> 8) & 0xFF;
	write(file, &tmp, 1);
	//write 0s to unused bits
	write(file, "\0\0\0\0", 4);
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
