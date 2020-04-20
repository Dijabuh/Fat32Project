//Header file for the project

//struct used
typedef struct
{
	char** parts;
	int numParts;
} pathparts;

struct BPB {
	unsigned short bytes_per_sec;
	unsigned char sec_per_clus;
	unsigned short rsvd_sec_cnt;
	unsigned char num_fats;
	unsigned int tot_sec_32;
	unsigned int fat_sz_32;
	unsigned int root_clus;
}__attribute__((packed));

struct DIRENTRY {
	unsigned char dir_name[11];
	unsigned char dir_attr;
	unsigned short hi_fst_clus;
	unsigned short lo_fst_clus;
	unsigned int dir_file_size;
}__attribute__((packed));

//function definitions
void splitString(pathparts* ret, char* str, char* c);
void mv_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster);
void getBPB(struct BPB* bpb, int file);
void info_cmd(struct BPB* bpb);
unsigned int get_data_location(unsigned int cluster, struct BPB* bpb);
void get_dir_entry(struct DIRENTRY* de, int file, unsigned int location);
unsigned int get_next_cluster(unsigned int cluster, struct BPB* bpb, int file);
void ls_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster);
void size_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster);
unsigned int cd_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster);
unsigned int get_next_empty_cluster(struct BPB* bpb, int file);
void set_next_cluster(struct BPB* bpb, int file, unsigned int cur_cluster, unsigned int next_cluster);
void create_new_dir(struct BPB* bpb, int file, pathparts* cmd, unsigned int dir_entry_location, unsigned int parent_clus, unsigned int dir_clus);
void mkdir_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster);
void create_new_file(struct BPB* bpb, int file, pathparts* cmd, unsigned int dir_entry_location, unsigned int parent_clus, unsigned int dir_clus);
void creat_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster);
int file_exists(struct BPB* bpb, int file, pathparts* cmd, unsigned int start_cluster);
void rm_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster);
void cp_cmd(struct BPB* bpb, int file, pathparts* cmd, unsigned int star_cluster);
