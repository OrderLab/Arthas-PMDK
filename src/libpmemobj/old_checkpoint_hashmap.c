/*
 * The Arthas Project
 *
 * Copyright (c) 2019, Johns Hopkins University - Order Lab.
 *
 *    All rights reserved.
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *
 */

#include "checkpoint_hashmap.h"
#include <time.h>

struct checkpoint_log *c_log;
int variable_count = 0;
void *pmem_file_ptr;
struct pool_info settings;
int non_checkpoint_flag = 0;
int sequence_number = 0;

void
init_checkpoint_log()
{
	non_checkpoint_flag = 1;
	/*size_t sisi3 = INT_MAX;
	      size_t sisi4 = INT_MAX;
	      sisi3 = 53*(sisi3 + sisi4);*/
	settings.pm_pool =
		pmemobj_create("/mnt/pmem/checkpoint.pm", "checkpoint",
			       PMEMOBJ_MIN_POOL * 1000, 0666);
	printf("pmemobjpool total size is %ld\n", PMEMOBJ_MIN_POOL * 1000);
	printf("pop is %p\n", settings.pm_pool);
	if (settings.pm_pool == NULL) {
		printf("ERROR CREATING POOL\n");
	}
	printf("create checkpoint tx pm ejueu\n");

	// Saving pmem_pool
	uint64_t size = sizeof(uint64_t);
	PMEMoid pmemoid = pmemobj_root(settings.pm_pool, size);
	uint64_t *root_num = pmemobj_direct(pmemoid);
	*root_num = (uint64_t)settings.pm_pool;
	pmem_persist(root_num, sizeof(uint64_t));
	TX_BEGIN(settings.pm_pool)
	{
		PMEMoid oid;
		printf("size of c log %ld\n", sizeof(struct checkpoint_log));
		oid = pmemobj_tx_zalloc(sizeof(struct checkpoint_log), 0);
		c_log = pmemobj_direct(oid);
		c_log->variable_count = 0;
		c_log->size = 5000010;
		size_t total_size = c_log->size * sizeof(struct node *);

		// Create Table
		oid = pmemobj_tx_zalloc(total_size, 2);
		c_log->list = pmemobj_direct(oid);
		printf("c_log list offset is %ld\n", oid.off);
		printf("c_log list here is %p\n", c_log->list);
		for (int i = 0; i < (int)c_log->size; i++) {
			c_log->list[i] = NULL;
		}
	}
	TX_ONABORT
	{
		printf("abort c_log creation\n");
	}
	TX_END
	non_checkpoint_flag = 0;
}

int
check_flag()
{
	return non_checkpoint_flag;
}

/*int search_for_offset(uint64_t pool_base, uint64_t offset){
  for(int i = 0; i < variable_count; i++){
    if(c_log->c_data[i].offset == offset){
      return i;
    }
  }
  return variable_count;
}

int search_for_address(const void * address){
  for(int i = 0; i < variable_count; i++){
    if(c_log->c_data[i].address == address){
      return i;
    }
  }
  return variable_count;
}

int check_address_length(const void *address, size_t size){
  uint64_t search_address = (uint64_t)address;
  for(int i = 0; i < variable_count; i++){
    uint64_t address_num = (uint64_t)c_log->c_data[i].address;
    uint64_t upper_bound = address_num + (uint64_t)c_log->c_data[i].size;
    if(search_address <= upper_bound && search_address >= address_num){
      return i;
    }
  }
  return -1;
}*/

void
shift_to_left(struct node *found_node)
{
	non_checkpoint_flag = 1;
	if (c_log == NULL) {
		printf("we return \n");
		return;
	}
	PMEMoid oid;
	PMEMoid free_oid;
	// printf("********SHIFT LEFT\n");
	for (int i = 0; i < MAX_VERSIONS - 1; i++) {
		free_oid = pmemobj_oid(found_node->c_data.data[i]);
		pmemobj_free(&free_oid);
		int ret = pmemobj_zalloc(settings.pm_pool, &oid,
					 found_node->c_data.size[i + 1], 1);
		if (ret != 0) {
			printf("ret here is %d %s\n", ret, pmemobj_errormsg());
			printf("var count is %d\n", variable_count);
		}
		found_node->c_data.data[i] = pmemobj_direct(oid);
		memcpy(found_node->c_data.data[i],
		       found_node->c_data.data[i + 1],
		       found_node->c_data.size[i + 1]);
		found_node->c_data.size[i] = found_node->c_data.size[i + 1];
		found_node->c_data.sequence_number[i] =
			found_node->c_data.sequence_number[i + 1];
	}
	non_checkpoint_flag = 0;
}

/*int check_offset(uint64_t offset, size_t size){
  uint64_t offset_upper_bound = offset + (uint64_t)size;
  for(int i = 0; i < variable_count; i++){
    uint64_t upper_bound = c_log->c_data[i].offset +
(uint64_t)c_log->c_data[i].size;
    if(offset >= c_log->c_data[i].offset && offset_upper_bound <= upper_bound){
      return i;
    }
  }
  return -1;
}*/

/*void revert_by_offset(const void *address, uint64_t offset, int
variable_index, int version, int type, size_t size){
  void *dest = (void *)address;
  if(offset == c_log->c_data[variable_index].offset){
    memcpy(dest, c_log->c_data[variable_index].data[version],
c_log->c_data[variable_index].size[version]);
  }
  else if(check_offset(offset, size) == variable_index){
    uint64_t clog_addr = c_log->c_data[variable_index].offset;
    uint64_t memcpy_offset = offset - clog_addr;
    memcpy(dest, (void *)( (uint64_t)c_log->c_data[variable_index].data[version]
+ memcpy_offset), size);
  }
}

void revert_by_address(const void *address, int variable_index, int version, int
type, size_t size){
  void *dest = (void *)address;
  if(address == c_log->c_data[variable_index].address){
    memcpy(dest, c_log->c_data[variable_index].data[version],
c_log->c_data[variable_index].size[version]);
  }
  else if(check_address_length(address, size) == variable_index){
    uint64_t search_address = (uint64_t)address;
    uint64_t address_num = (uint64_t)c_log->c_data[variable_index].address;
    uint64_t offset = search_address - address_num;
    memcpy(dest, (void *)( (uint64_t)c_log->c_data[variable_index].data[version]
+ offset), size);
  }
}*/

void
insert_value(const void *address, size_t size, const void *data_address,
	     uint64_t offset)
{
	non_checkpoint_flag = 1;
	// printf("INSERT VALUE value of size %ld offset is %ld seq num is
	// %d\n",
	//          size, offset, sequence_number);
	if (c_log == NULL) {
		return;
	}

	clock_t start;
	clock_t end;
	double time;
	// int version_index = 0;
	PMEMoid oid;
	// Look for address in hashmap
	struct node *found_node = lookup(offset);
	struct checkpoint_data insert_data;
	if (found_node == NULL) {
		// We need to insert node for address
		start = clock();
		c_log->variable_count = c_log->variable_count + 1;
		variable_count = variable_count + 1;
		insert_data.address = address;
		insert_data.offset = offset;
		insert_data.size[0] = size;
		insert_data.version = 0;
		insert_data.sequence_number[0] = sequence_number;
		end = clock();
		time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("New node time assignment %f\n", time);
		start = clock();
		__atomic_fetch_add(&sequence_number, 1, __ATOMIC_SEQ_CST);
		end = clock();
		time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("New node time atomic inst %f\n", time);
		start = clock();
		int ret = pmemobj_zalloc(settings.pm_pool, &oid, size, 1);
		if (ret != 0) {
			printf("ret here is %d %s\n", ret, pmemobj_errormsg());
			printf("var count is %d\n", variable_count);
		}
		end = clock();
		time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("New node time pmem zalloc %f\n", time);
		start = clock();
		insert_data.data[0] = pmemobj_direct(oid);
		memcpy(insert_data.data[0], data_address, size);
		end = clock();
		time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("New node time memcpy%f\n", time);
		start = clock();
		insert(offset, insert_data);
		end = clock();
		time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("New node time after insert %f\n", time);
	} else if (found_node->c_data.data_type == -1) {
		// start = clock();
		c_log->variable_count = c_log->variable_count + 1;
		variable_count = variable_count + 1;
		insert_data.address = address;
		insert_data.offset = offset;
		insert_data.size[0] = size;
		insert_data.version = 0;
		insert_data.sequence_number[0] = sequence_number;
		__atomic_fetch_add(&sequence_number, 1, __ATOMIC_SEQ_CST);
		int ret = pmemobj_zalloc(settings.pm_pool, &oid, size, 1);
		if (ret != 0) {
			printf("ret here is %d %s\n", ret, pmemobj_errormsg());
			printf("var count is %d\n", variable_count);
		}
		insert_data.data[0] = pmemobj_direct(oid);
		memcpy(insert_data.data[0], data_address, size);
		insert(offset, insert_data);
		// end = clock();
		// time = (double)(end - start)/CLOCKS_PER_SEC;
		// printf("realloc node time %f\n", time);
	} else {
		start = clock();
		if (found_node->c_data.version + 1 == MAX_VERSIONS) {
			shift_to_left(found_node);
		} else {
			found_node->c_data.version += 1;
		}
		end = clock();
		time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("Existing node time shift or assign %f\n", time);
		start = clock();
		int data_index = found_node->c_data.version;
		found_node->c_data.address = address;
		found_node->c_data.size[data_index] = size;
		end = clock();
		time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("Existing node time assignment %f\n", time);
		start = clock();
		int ret = pmemobj_zalloc(settings.pm_pool, &oid, size, 1);
		if (ret != 0) {
			printf("ret here is %d %s\n", ret, pmemobj_errormsg());
			printf("variable count is %d\n", variable_count);
		}
		end = clock();
		time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("Existing node time alloc %f\n", time);
		start = clock();
		found_node->c_data.data[data_index] = pmemobj_direct(oid);
		memcpy(found_node->c_data.data[data_index], data_address, size);
		found_node->c_data.sequence_number[data_index] =
			sequence_number;
		__atomic_fetch_add(&sequence_number, 1, __ATOMIC_SEQ_CST);
		end = clock();
		time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("Existing node time memcpy + atomic fetch%f\n", time);
	}

	non_checkpoint_flag = 0;
}

void
checkpoint_realloc(void *new_ptr, void *old_ptr, uint64_t new_offset,
		   uint64_t old_offset)
{
	printf("realloc invokation\n");
	printf("old offset is %ld new offset is %ld\n", old_offset, new_offset);
	struct node *temp = lookup(old_offset);
	if (!temp) {
		printf("realloc has failed\n");
	}
	// int index  = search_for_offset(0, old_offset);
	// if (index  < 0){
	//  printf("realloc failed\n");
	//  return;
	//}
	// printf("variable count is %d\n", index);
	/// printf("old offset is %ld\n", c_log->c_data[index].offset);
	// We cannot use offsets since new pointer has not been labeled yet.
	// c_log->c_data[variable_count].address = new_ptr;
	temp->c_data.new_checkpoint_entry = new_offset;
	// c_log->c_data[index].offset = new_offset;
	// printf("new offset is %ld\n", c_log->c_data[index].offset);
	struct checkpoint_data insert_data;
	insert_data.old_checkpoint_entry = old_offset;
	insert_data.data_type = -1;
	insert(new_offset, insert_data);
}

void
checkpoint_free(uint64_t off)
{
	struct node *temp = lookup(off);
	if (!temp)
		return;
	temp->c_data.free_flag = 1;
}

/*void checkpoint_realloc(void *new_ptr, void *old_ptr, uint64_t new_offset,
			uint64_t old_offset){
  printf("reallocaiton happened\n");
  int old_index = search_for_offset(0, old_offset);
  c_log->c_data[old_index].free_flag = 1;
  // FIXME: This is all under the assumption that you will call
  // tx_add_range to realloc'd data immediately afterwards.
  // Should I just add an empty address?
  int variable_index = search_for_offset(0, new_offset);
  c_log->c_data[old_index].new_checkpoint_entry = new_offset;
  c_log->c_data[variable_index].version = 0;
  c_log->c_data[variable_index].offset = new_offset;
  c_log->c_data[variable_index].old_checkpoint_entry = old_offset;
}*/

/*int sequence_comparator(const void *v1, const void * v2){

  struct single_data *s1 = (struct single_data *)v1;
  struct single_data *s2 = (struct single_data *)v2;
  printf("comparing shit\n");
  printf("seq num is %d\n", s2->sequence_number);
  printf("seq num is %d\n", s1->sequence_number);
  if (s1->sequence_number < s2->sequence_number)
	return -1;
  else if (s1->sequence_number > s2->sequence_number)
	return 1;
  else
	return 0;
}

void print_sequence_array(struct single_data *ordered_data, size_t total_size){
  printf("**************************\n\n");
  for(size_t i = 0; i < total_size; i++){
    printf("address %p sequence num %d\n", ordered_data[i].address,
ordered_data[i].sequence_number);
    if(ordered_data[i].size == 4){
      printf("int data is %d\n", *(int *)ordered_data[i].data);
    }else{
      printf("double data is %f\n", *(double *)ordered_data[i].data);
    }
  }
}

void order_by_sequence_num(struct single_data * ordered_data, size_t
*total_size){
  //struct single_data ordered_data[MAX_VARIABLES];
  for(int i = 0; i < variable_count; i++){
    int data_index = c_log->c_data[i].version;
    for(int j = 0; j <= data_index; j++){
     ordered_data[*total_size].address = c_log->c_data[i].address;
     ordered_data[*total_size].offset = c_log->c_data[i].offset;
     ordered_data[*total_size].data = malloc(c_log->c_data[i].size[j]);
     memcpy(ordered_data[*total_size].data, c_log->c_data[i].data[j],
c_log->c_data[i].size[j]);
     ordered_data[*total_size].size = c_log->c_data[i].size[j];
     ordered_data[*total_size].version = j;
     ordered_data[*total_size].sequence_number =
c_log->c_data[i].sequence_number[j];
     *total_size = *total_size + 1;
    }
  }

  qsort(ordered_data, *total_size, sizeof(struct single_data),
sequence_comparator);
}*/

int
hashCode(uint64_t offset)
{
	int ret_val;
	if (offset < 0) {
		ret_val = (int)(offset % c_log->size);
		ret_val = -ret_val;
	}
	ret_val = (int)(offset % c_log->size);
	return ret_val;
}

void
insert(uint64_t offset, struct checkpoint_data c_data)
{
	PMEMoid oid;
	int pos = hashCode(offset);
	struct node *list = c_log->list[pos];
	struct node *temp = list;
	uint64_t old_offset = 0;
	while (temp) {
		if (temp->offset == offset) {
			if (temp->c_data.old_checkpoint_entry != 0) {
				old_offset = temp->c_data.old_checkpoint_entry;
			}
			temp->c_data = c_data;
			temp->c_data.old_checkpoint_entry = old_offset;
			return;
		}
		temp = temp->next;
	}
	// Need to create a new insertion
	int ret =
		pmemobj_zalloc(settings.pm_pool, &oid, sizeof(struct node), 2);
	if (ret != 0) {
		printf("var count is %d\n", variable_count);
	}
	struct node *newNode = pmemobj_direct(oid);
	newNode->offset = offset;
	newNode->c_data = c_data;
	newNode->next = list;
	c_log->list[pos] = newNode;
}

struct node *
lookup(uint64_t offset)
{
	int pos = hashCode(offset);
	struct node *list = c_log->list[pos];
	struct node *temp = list;
	// int num = 0;
	while (temp) {
		// if(num > 0)
		// printf("num lookup is %d\n", num);
		if (temp->offset == offset) {
			return temp;
		}
		// num++;
		temp = temp->next;
	}
	return NULL;
}

void
print_checkpoint_log()
{
	// if(sequence_number <= 4131080 && sequence_number != 3999993)
	//  return;
	printf("**************\n\n");
	struct node *list;
	struct node *temp;
	for (int i = 0; i < (int)c_log->size; i++) {
		list = c_log->list[i];
		temp = list;
		while (temp) {
			// if(temp->c_data.offset == 223388685424 ||
			// temp->c_data.offset == 223451520752 || i == 1073852){
			printf("position is %d\n", i);
			printf("address is %p offset is %ld\n",
			       temp->c_data.address, temp->offset);
			int data_index = temp->c_data.version;
			printf("number of versions is %d\n",
			       temp->c_data.version);
			printf("old checkpoint %ld new cp %ld\n",
			       temp->c_data.old_checkpoint_entry,
			       temp->c_data.new_checkpoint_entry);
			for (int j = 0; j <= data_index; j++) {
				printf("version is %d size is %ld value is %f or %d\n",
				       j, temp->c_data.size[j],
				       *((double *)temp->c_data.data[j]),
				       *((int *)temp->c_data.data[j]));
				int *ref =
					(int *)((uint64_t)temp->c_data.data[j] +
						4);
				printf("refcount could be %d\n", *ref);
			}
			//}
			temp = temp->next;
		}
	}
	printf("the variable count is %d %d\n", variable_count,
	       c_log->variable_count);
}
