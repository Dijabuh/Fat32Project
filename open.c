#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

int check_entry(opentable* table, unsigned int start_cluster){

	int i;
	for(i = 0; i < table->size; ++i){
		if(table->opened[i].start_cluster == start_cluster)
			return i + 1;
	}
	return 0;
}

void add_entry(opentable* table, unsigned int start_cluster, char* mode){
	if(table->size){
		table->opened = (openentry*)realloc(table->opened, sizeof(openentry)*(table->size+1));
		table->opened[table->size].start_cluster = start_cluster;
		table->opened[table->size].mode = (char*)malloc(sizeof(mode));
		strcpy(table->opened[table->size].mode, mode);
	}
	else{
		table->opened = (openentry*)malloc(sizeof(openentry));
		table->opened[0].start_cluster = start_cluster;
		table->opened[0].mode = (char*)malloc(sizeof(mode));
		strcpy(table->opened[0].mode, mode);
	}
	++table->size;
}

void copy_entry(openentry* dest, openentry* src){
	dest->start_cluster = src->start_cluster;
	strcpy(dest->mode, src->mode);
}

int remove_entry(opentable* table, unsigned int start_cluster){
	int pos = check_entry(table, start_cluster);
	if(pos == 0) return 0;
	
	for(--pos; pos+1 < table->size; ++pos)
		copy_entry(&table->opened[pos], &table->opened[pos+1]);

	free(table->opened[pos].mode);
	table->opened = (openentry*)realloc(table->opened, sizeof(openentry)*--table->size);
	return 1;
}

void print(opentable* table){
	int i = 0;
	for(; i < table->size; ++i){
		printf("%d: %d, %s\n", i, table->opened[i].start_cluster, table->opened[i].mode);
	}
}

void open_cmd(struct BPB* bpb, int file, pathparts* cmd, 
unsigned int start_cluster, opentable* table){

	if(cmd->numParts != 3){
		printf("Wrong number of arguments for open\n");
		return;
	}

	// Checks and stores mode
	if(strcmp("r", cmd->parts[2]) && strcmp("w", cmd->parts[2]) &&
	strcmp("rw", cmd->parts[2]) && strcmp("wr", cmd->parts[2])){
		printf("Invalid open mode\n");
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
	if(check_entry(table, file_clus)){
		printf("%s is already open\n", cmd->parts[1]);
		return;
	}

	add_entry(table, file_clus, cmd->parts[2]);
}

void close_cmd(struct BPB* bpb, int file, pathparts* cmd, 
unsigned int start_cluster, opentable* table){
	
	if(cmd->numParts != 2){
		printf("Wrong number of arguments for close\n");
		return;
	}
	// finds existing file by start cluster
	int file_clus;
	file_clus = get_file_clus(bpb, file, cmd, start_cluster);
	if(file_clus == 0){
		printf("%d not found\n", file_clus);
		return;
	}
	if(remove_entry(table, file_clus) == 0)
		printf("No entry for %s found\n", cmd->parts[1]);
}
