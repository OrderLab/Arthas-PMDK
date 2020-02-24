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

#define INT_CHECKPOINT 0
#define DOUBLE_CHECKPOINT 1
#define STRING_CHECKPOINT 2
#define BOOL_CHECKPOINT 3

struct checkpoint_data {
  const void *address;
  uint64_t offset;
  void *data[MAX_VERSIONS];
  size_t size[MAX_VERSIONS];
  int version;
  int data_type;
};

struct checkpoint_log{
  struct checkpoint_data c_data[MAX_VARIABLES];
};

extern struct checkpoint_log c_log;
extern int variable_count;
extern void *pmem_file_ptr;

void shift_to_left(int variable_index);
int check_address_length(const void *address, size_t size);
int search_for_offset(uint64_t pool_base, uint64_t offset);
int search_for_address(const void *address);
void insert_value(const void *address, int variable_index, size_t size, const void *data_address, uint64_t offset);
void print_checkpoint_log(void);
void revert_by_address(const void *address, int variable_index, int version, int type, size_t size);
#endif
