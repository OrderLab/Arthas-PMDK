#include "checkpoint_generic.h"

struct checkpoint_log c_log;
int variable_count = 0;

int search_for_address(const void * address){
  for(int i = 0; i < variable_count; i++){
    if(c_log.c_data[i].address == address){
      return i;
    }
  }
  printf("variable count in search is %d\n", variable_count);
  return variable_count;
}

void insert_value(const void *address, int variable_index, size_t size){
  if(variable_index == 0 && variable_count == 0){
    variable_count = variable_count + 1;
    c_log.c_data[variable_index].address = address;
    c_log.c_data[variable_index].size[0] = size;
    c_log.c_data[variable_index].version = 0;
    printf("before memcpy variable count is %d\n", variable_count);
    c_log.c_data[variable_index].data[0] = malloc(size);
    memcpy(c_log.c_data[variable_index].data[0], address, size);
  }
  else{
    if(variable_count == variable_index){
      c_log.c_data[variable_index].version = 0;
      variable_count++;
    }
    else{
      c_log.c_data[variable_index].version += 1;
    }
    int data_index = c_log.c_data[variable_index].version;
    c_log.c_data[variable_index].size[data_index] = size;
    c_log.c_data[variable_index].data[data_index] = malloc(size);
    memcpy(c_log.c_data[variable_index].data[data_index], address, size);
  }

}

void print_checkpoint_log(){
  printf("print begin\n");
  for(int i = 0; i < variable_count; i++){
    printf("address is %p\n", c_log.c_data[i].address);
    int data_index = c_log.c_data[i].version;
    for(int j = 0; j <= data_index; j++){
      printf("version is %d size is %ld value is %s or %d\n", j , c_log.c_data[i].size[j], (char *)c_log.c_data[i].data[j], *((int *)c_log.c_data[i].data[j]));
    }
  }
}
