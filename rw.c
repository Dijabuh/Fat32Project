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
	// checks file type
	int dir_attr;
	dir_attr = get_file_type(bpb, file, cmd, start_cluster);
	if(dir_attr != 2){
		printf("%s is wrong file type\n", cmd->parts[1]);
		return;
	}
	// checks if entry exists
	if(!check_entry(table, file_clus)){
		printf("%s isn't opened\n", cmd->parts[1]);
		return;
	}
}
