#include "headers.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>

void getBPB(struct BPB* bpb, int file) {
	unsigned char ch1, ch2, ch3, ch4;
	//bytes per sec
	lseek(file, 11, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	bpb->bytes_per_sec = (ch2 << 8) + ch1;

	//sec per cluster
	read(file, &ch1, 1);
	bpb->sec_per_clus = ch1;

	//reserved sec count
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	bpb->rsvd_sec_cnt = (ch2 << 8) + ch1;

	//num fats
	read(file, &ch1, 1);
	bpb->num_fats = ch1;

	//tot sec 32
	lseek(file, 32, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	read(file, &ch3, 1);
	read(file, &ch4, 1);
	bpb->tot_sec_32 = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;

	//fat size 32
	lseek(file, 36, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	read(file, &ch3, 1);
	read(file, &ch4, 1);
	bpb->fat_sz_32 = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;

	//root cluster
	lseek(file, 44, SEEK_SET);
	read(file, &ch1, 1);
	read(file, &ch2, 1);
	read(file, &ch3, 1);
	read(file, &ch4, 1);
	bpb->root_clus = (ch4 << 24) + (ch3 << 16) + (ch2 << 8) + ch1;
}

void info_cmd(struct BPB* bpb) {
	printf("Bytes Per Sector: %hu\n", bpb->bytes_per_sec);
	printf("Sectors Per Cluster: %hhu\n", bpb->sec_per_clus);
	printf("Reserved Sector Count: %hu\n", bpb->rsvd_sec_cnt);
	printf("Number of Fats: %hhu\n", bpb->num_fats);
	printf("Total Sectors: %u\n", bpb->tot_sec_32);
	printf("Fat Size: %u\n", bpb->fat_sz_32);
	printf("Root Cluster: %u\n", bpb->root_clus);
}
