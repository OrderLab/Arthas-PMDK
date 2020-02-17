#ifndef CHECKPOINT_GENERIC_H
#define CHECKPOINT_GENERIC_H 1

#define MAX_VARIABLES 1000
#define MAX_VERSIONS 5
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>

struct checkpoint_data {
  const void *address;
  uint64_t offset;
  void *data[MAX_VERSIONS];
  size_t size[MAX_VERSIONS];
  int version;
};

struct checkpoint_log{
  struct checkpoint_data c_data[MAX_VARIABLES];
};

extern struct checkpoint_log c_log;
extern int variable_count;

int search_for_offset(uint64_t pool_base, uint64_t offset);
int search_for_address(const void *address);
void insert_value(const void *address, int variable_index, size_t size);
void print_checkpoint_log(void);
#endif
