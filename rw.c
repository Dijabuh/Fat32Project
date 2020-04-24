#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

void read_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster, opentable* table){
	
	if(cmd->numParts != 4){
		printf("Wrong number of arguments for read\n");
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
	
	int cluster_offset = offset / bpb->bytes_per_sec;
	unsigned int cluster = get_file_clus(bpb, file, cmd, start_cluster);
	offset %= bpb->bytes_per_sec;
	while(cluster_offset > 0){
		cluster = get_next_cluster(cluster, bpb, file);
		--cluster_offset;
	}
	
	unsigned int location = get_data_location(cluster, bpb) + offset;
	lseek(file, location, SEEK_SET);
	int i;
	char tmp;

	for(i = 0; i < size; ++i){
		if(i % bpb->bytes_per_sec == 0 && i > 0){
			cluster = get_next_cluster(cluster, bpb, file);
			location = get_data_location(cluster, bpb);
			lseek(file, location, SEEK_SET);
		}
		read(file, &tmp, 1);
		printf("%c", tmp);
	}
	printf("\n");
}

void write_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster, opentable* table){

	// TESTS
	if(cmd->numParts != 5){
		printf("Wrong number of arguments for write\n");
		return;
	}
	
	// finds existing file by start cluster
	int file_clus;
	file_clus = get_file_clus(bpb, file, cmd, start_cluster);
	if(file_clus == 0){
		printf("%s not found\n", cmd->parts[1]);
		return;
	}
	if(get_file_type(bpb, file, cmd, start_cluster) == 1){
		printf("File is a directory\n");
		return;
	}
	int entry_num;
	entry_num = check_entry(table, file_clus);
	if(entry_num == 0){
		printf("File not open\n");
		return;
	}
	if(table->opened[entry_num-1].mode[0] == 'R'){
		printf("File not open to write\n");
		return;
	}
	struct DIRENTRY de;
	get_dir(bpb, file, cmd, start_cluster, &de);
	if(atoi(cmd->parts[2]) > de.dir_file_size){
		printf("Offset out of bounds\n");
		return;
	}

	// ALLOCATES NEW CLUSTERS
	int offset, size;
	offset = atoi(cmd->parts[2]);
	size = atoi(cmd->parts[3]);
	
	if(offset + size > de.dir_file_size){
		
		int current_clusters, final_clusters;

		// finds amount of clusters there are and will be
		current_clusters = de.dir_file_size / bpb->bytes_per_sec;
		if(de.dir_file_size % bpb->bytes_per_sec > 0) ++current_clusters;
		
		final_clusters = (offset + size) / bpb->bytes_per_sec;
		if((offset+size) % bpb->bytes_per_sec > 0) ++final_clusters;

		// finds the last cluster before needing new ones
		int extra_alloc, current_cluster;
		extra_alloc = final_clusters - current_clusters;
		
		current_cluster = file_clus;
		for(; current_clusters > 0; --current_clusters)
			current_cluster = get_next_cluster(current_cluster, bpb, file);
		
		// generates new clusters
		for(; extra_alloc > 0; --extra_alloc){
			unsigned next_dir_clus = get_next_empty_cluster(bpb, file);
			set_next_cluster(bpb, file, current_cluster, next_dir_clus);
			set_next_cluster(bpb, file, next_dir_clus, 0xFFFFFFFF);
			unsigned int new_dir_clus = get_next_empty_cluster(bpb, file);
			set_next_cluster(bpb, file, new_dir_clus, 0xFFFFFFFF);
		}
		set_dir_size(bpb, file, cmd, start_cluster, (offset + size));
	}

	// CALCULATES CLUSTERS, OFFSET, SIZE
	int cluster_offset;
	cluster_offset = offset / bpb->bytes_per_sec;
	unsigned int cluster = get_file_clus(bpb, file, cmd, start_cluster);
	offset %= bpb->bytes_per_sec;
	
	while(cluster_offset > 0){
		cluster = get_next_cluster(cluster, bpb, file);
		--cluster_offset;
	}
	
	unsigned int location = get_data_location(cluster, bpb) + offset;
	lseek(file, location, SEEK_SET);
	int i, w_count;

	for(i = 0, w_count = 1; i < size; ++i, ++w_count){
		if(i % bpb->bytes_per_sec == 0 && i > 0){
			cluster = get_next_cluster(cluster, bpb, file);
			location = get_data_location(cluster, bpb);
			lseek(file, location, SEEK_SET);
		}
		if(w_count < sizeof(cmd->parts[4])-1) // could be indexing problem
			write(file, &cmd->parts[4][i], 1);
		else
			write(file, 0, 1);
	}
}
