#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

void read_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster, opentable* table){
	
	if(cmd->numParts != 4){
		printf("Wrong number of arguments for open\n");
		return;
	}
	// finds existing file by start cluster
	int file_clus;
	file_clus = get_file_clus(bpb, file, cmd, start_cluster);
	if(file_clus == 0){
		printf("%s not found\n", cmd->parts[1]);
		return;
	}
	// checks if entry exists
	if(!check_entry(table, file_clus)){
		printf("%s isn't opened\n", cmd->parts[1]);
		return;
	}

	int entry;
	entry = check_entry(table, file_clus);
	if(strcmp(table->opened[entry-1].mode, "w") == 0){
		printf("File is opened as write only\n");
		return;
	}
	
	struct DIRENTRY de;
	get_dir(bpb, file, cmd, start_cluster, &de);
	
	int offset, size;
	offset = atoi(cmd->parts[2]);
	size = atoi(cmd->parts[3]);
	if(offset >= de.dir_file_size){
		printf("Offset out of bounds\n");
		return;
	}
	if(offset + size >= de.dir_file_size)
		size = de.dir_file_size - offset;
	
	int cluster_offset = offset / 512;
	unsigned int cluster = get_file_clus(bpb, file, cmd, start_cluster);
	offset %= 512;
	while(cluster_offset > 0){
		cluster = get_next_cluster(cluster, bpb, file);
		--cluster_offset;
	}
	
	unsigned int location = get_data_location(cluster, bpb) + offset;
	lseek(file, location, SEEK_SET);
	int i;
	char tmp;

	for(i = 0; i < size; ++i){
		if(i % 512 == 0 && i > 0){
			cluster = get_next_cluster(cluster, bpb, file);
			location = get_data_location(cluster, bpb);
			lseek(file, location, SEEK_SET);
		}
		read(file, &tmp, 1);
		printf("%c", tmp);
	}
	printf("\n");
}
