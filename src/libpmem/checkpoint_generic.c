#include "checkpoint_generic.h"

struct checkpoint_log c_log;
int variable_count = 0;
void *pmem_file_ptr;

int search_for_offset(uint64_t pool_base, uint64_t offset){
  for(int i = 0; i < variable_count; i++){
    if(c_log.c_data[i].offset == offset){
      return i;
    }
  }
  return variable_count;
}

int search_for_address(const void * address){
  for(int i = 0; i < variable_count; i++){
    if(c_log.c_data[i].address == address){
      return i;
    }
  }
  printf("variable count in search is %d\n", variable_count);
  return variable_count;
}

int check_address_length(const void *address, int size){
  uint64_t search_address = (uint64_t)address;
  for(int i = 0; i < variable_count; i++){
    uint64_t address_num = (uint64_t)c_log.c_data[i].address;
    uint64_t upper_bound = address_num + (uint64_t)c_log.c_data[i].size;
    printf("sa: %ld ub:%ld ad_num: %ld\n", search_address, upper_bound, address_num);
    if(search_address <= upper_bound && search_address >= address_num){
      return i;
    }
  }
  return -1;
}

void shift_to_left(int variable_index){
  for(int i = 0; i < MAX_VERSIONS -1; i++){
    free(c_log.c_data[variable_index].data[i]);
    c_log.c_data[variable_index].data[i] = malloc(c_log.c_data[variable_index].size[i+1]);
    memcpy(c_log.c_data[variable_index].data[i],
    c_log.c_data[variable_index].data[i+1], c_log.c_data[variable_index].size[i+1]);
    c_log.c_data[variable_index].size[i] = c_log.c_data[variable_index].size[i+1];
  }
}

void insert_value(const void *address, int variable_index, size_t size, const void *data_address
, uint64_t offset){
  if(variable_index == 0 && variable_count == 0){
    variable_count = variable_count + 1;
    c_log.c_data[variable_index].address = address;
    c_log.c_data[variable_index].offset = offset;
    c_log.c_data[variable_index].size[0] = size;
    c_log.c_data[variable_index].version = 0;
    printf("before memcpy variable count is %d\n", variable_count);
    c_log.c_data[variable_index].data[0] = malloc(size);
    memcpy(c_log.c_data[variable_index].data[0], data_address, size);
  }
  else{
    if(variable_count == variable_index){
      c_log.c_data[variable_index].version = 0;
      c_log.c_data[variable_index].address = address;
      c_log.c_data[variable_index].offset = offset;
      variable_count++;
    }
    else{
      if(c_log.c_data[variable_index].version + 1 == MAX_VERSIONS){
        //we need to shift everything in c_log.c_data[variable_index] to the left
        shift_to_left(variable_index);
      }
      else{
        c_log.c_data[variable_index].version += 1;
      }
    }
    int data_index = c_log.c_data[variable_index].version;
    c_log.c_data[variable_index].size[data_index] = size;
    c_log.c_data[variable_index].data[data_index] = malloc(size);
    memcpy(c_log.c_data[variable_index].data[data_index], data_address, size);
  }

}

void print_checkpoint_log(){
  printf("print begin\n");
  for(int i = 0; i < variable_count; i++){
    printf("address is %p\n", c_log.c_data[i].address);
    int data_index = c_log.c_data[i].version;
    for(int j = 0; j <= data_index; j++){
      printf("version is %d size is %ld value is %s or %f or %d offset us %ld\n", j , c_log.c_data[i].size[j], (char *)c_log.c_data[i].data[j]
      ,*((double *)c_log.c_data[i].data[j]) ,*((int *)c_log.c_data[i].data[j]),  c_log.c_data[i].offset);
    }
  }
}
