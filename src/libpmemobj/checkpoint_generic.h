#ifndef CHECKPOINT_GENERIC_H
#define CHECKPOINT_GENERIC_H 1

#define MAX_VARIABLES 1000
#define MAX_VERSIONS 3
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include "libpmemobj.h"
#include <limits.h>
//#include "pmem.h"

#define INT_CHECKPOINT 0
#define DOUBLE_CHECKPOINT 1
#define STRING_CHECKPOINT 2
#define BOOL_CHECKPOINT 3

struct pool_info {
  PMEMobjpool *pm_pool;
};

struct single_data {
  const void *address;
  uint64_t offset;
  void *data;
  size_t size;
  int sequence_number;
  int version;
  int data_type;
};

struct checkpoint_data {
  const void *address;
  uint64_t offset;
  void *data[MAX_VERSIONS];
  size_t size[MAX_VERSIONS];
  int version;
  int data_type;
  int sequence_number[MAX_VERSIONS];
  uint64_t old_checkpoint_entry;
  int free_flag;
  //uint64_t old_checkpoint_entries[MAX_VERSIONS];
  //int old_checkpoint_counter;
};

struct checkpoint_log{
  struct checkpoint_data c_data[MAX_VARIABLES];
  int variable_count;
};

extern struct checkpoint_log *c_log;
extern int variable_count;
extern void *pmem_file_ptr;
extern struct pool_info settings ;
extern int non_checkpoint_flag;

void init_checkpoint_log(void);
int check_flag(void);
void write_flag(char c);
void shift_to_left(int variable_index);
int check_address_length(const void *address, size_t size);
int search_for_offset(uint64_t pool_base, uint64_t offset);
int search_for_address(const void *address);
void insert_value(const void *address, int variable_index, size_t size, const void *data_address, uint64_t offset);
void print_checkpoint_log(void);
void revert_by_address(const void *address, int variable_index, int version, int type, size_t size);
int check_offset(uint64_t offset, size_t size);
void revert_by_offset(const void *address, uint64_t offset, int variable_index, int version, int type, size_t size);
void order_by_sequence_num(struct single_data * ordered_data, size_t *total_size);
int sequence_comparator(const void *v1, const void * v2);
void print_sequence_array(struct single_data *ordered_data, size_t total_size);
void checkpoint_realloc(void *new_ptr, void *old_ptr, uint64_t new_offset, uint64_t old_offset);
void checkpoint_free(uint64_t off);
#endif
