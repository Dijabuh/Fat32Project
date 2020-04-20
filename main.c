#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>
#include "headers.h"

int main(int argc, char** argv) {
	
	struct BPB bpb;
	char* buf;
	buf = (char*) malloc(sizeof(char) * 101);
	
	if(argc != 2) {
		printf("Wrong number of arguments\n");
		return 0;
	}

	int file = open(argv[1], O_RDWR);

	getBPB(&bpb, file);

	opentable table;
	table.size = 0;
	unsigned int start_cluster = 2;

	while(1) {
		printf("$ ");
		fgets(buf, 100, stdin);
		strtok(buf, "\n");

		pathparts cmd;
		splitString(&cmd, buf, " ");

		if(strcmp(cmd.parts[0], "exit") == 0) {
			return 0;
		}
		else if(strcmp(cmd.parts[0], "info") == 0) {
			info_cmd(&bpb);
		}
		else if(strcmp(cmd.parts[0], "ls") == 0) {
			ls_cmd(&bpb, file, &cmd, start_cluster);
		}
		else if(strcmp(cmd.parts[0], "size") == 0) {
			size_cmd(&bpb, file, &cmd, start_cluster);
		}
		else if(strcmp(cmd.parts[0], "cd") == 0) {
			int ret = cd_cmd(&bpb, file, &cmd, start_cluster);
			if(ret > 0) {
				start_cluster = ret;
			}
		}
		else if(strcmp(cmd.parts[0], "mkdir") == 0) {
			mkdir_cmd(&bpb, file, &cmd, start_cluster);
		}
		else if(strcmp(cmd.parts[0], "creat") == 0) {
			creat_cmd(&bpb, file, &cmd, start_cluster);
		}
		else if(strcmp(cmd.parts[0], "mv") == 0) {
			mv_cmd(&bpb, file, &cmd, start_cluster);
		}
		else if(strcmp(cmd.parts[0], "rm") == 0) {
			rm_cmd(&bpb, file, &cmd, start_cluster);
		}
		else if(strcmp(cmd.parts[0], "cp") == 0) {
			cp_cmd(&bpb, file, &cmd, start_cluster);
		}
		else if(strcmp(cmd.parts[0], "open") == 0) {
			open_cmd(&bpb, file, &cmd, start_cluster, &table);
		}
		else if(strcmp(cmd.parts[0], "close") == 0) {
			close_cmd(&bpb, file, &cmd, start_cluster, &table);
		}
		else if(strcmp(cmd.parts[0], "read") == 0) {
			read_cmd(&bpb, file, &cmd, start_cluster, &table);
		}
		else
			printf("Command not found\n");
	}

	return 0;
}
