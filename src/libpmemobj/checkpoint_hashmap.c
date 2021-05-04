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

//struct checkpoint_log *c_log;
struct checkpoint_log *c_log[PM_FILES];
int variable_count = 0;
void *pmem_file_ptr;
void *checkpoint_file_curr[PM_FILES];
void *checkpoint_file_address[PM_FILES];
//void *checkpoint_file_curr;
//void *checkpoint_file_address;

pthread_mutex_t mutex;
struct pool_info settings;
int non_checkpoint_flag = 0;
int sequence_number = 0;
size_t mapped_len;
uint64_t total_alloc = 0;
void *mmap_address = NULL;
int file_count = 0;

struct file_log *flog;

void
init_checkpoint_log(void *addr, size_t poolsize)
{
	// return;
	printf("init pmem checkpoint\n");
	if (flog){
          printf("we need to handle multiple files\n");
	  //return;
        }
        else{
          flog = malloc(sizeof(struct file_log));
          flog->size = PM_FILES;
          flog->list = (struct file_node **)malloc(sizeof(struct file_node *) * PM_FILES);
          for(int i = 0; i < PM_FILES; i++){
            flog->list[i] = NULL;
          }
        }

	int pos = file_count;
        fileInsert((uint64_t)addr, pos);
        __atomic_fetch_add(&file_count, 1, __ATOMIC_SEQ_CST);
	non_checkpoint_flag = 1;
	c_log[pos] = malloc(sizeof(struct checkpoint_log));
	int is_pmem;
        char filename[40];
	snprintf(filename, 40, "/mnt/pmem/pmem_checkpoint%d.pm", pos);
	if ((c_log[pos] = (struct checkpoint_log *)pmem_map_file(
		     filename, PMEM_LENGTH,
		     PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem)) == NULL) {
		perror("pmem_map_file");
		exit(1);
	}
	c_log[pos]->variable_count = 0;

	checkpoint_file_address[pos] = c_log[pos];
	checkpoint_file_curr[pos] = (void *)((uint64_t)checkpoint_file_address[pos] +
					sizeof(struct checkpoint_log));
	void *old_pool_ptr = (void *)checkpoint_file_address[pos];
	uint64_t old_pool = (uint64_t)old_pool_ptr;
	memcpy(checkpoint_file_curr[pos], &old_pool, (sizeof(uint64_t)));
	checkpoint_file_curr[pos] =
		(void *)((uint64_t)checkpoint_file_curr[pos] + sizeof(uint64_t));
	c_log[pos]->list = checkpoint_file_curr[pos];
	printf("c_log->list is %p %ld\n", c_log[pos]->list,
	       (uint64_t)c_log[pos]->list - (uint64_t)c_log[pos]);
	checkpoint_file_curr[pos] =
		(void *)((uint64_t)checkpoint_file_curr[pos] + sizeof(c_log[pos]->list));
	c_log[pos]->size = 5000010;
	for (int i = 0; i < (int)c_log[pos]->size; i++) {
		c_log[pos]->list[i] = NULL;
		checkpoint_file_curr[pos] = (void *)((uint64_t)checkpoint_file_curr[pos] +
						sizeof(struct node *));
	}
	if (is_pmem)
		pmem_persist(checkpoint_file_address[pos], mapped_len);
	else
		pmem_msync(checkpoint_file_address[pos], mapped_len);

	non_checkpoint_flag = 0;
}

void mmap_set(void *address){
  mmap_address = address;
}

uint64_t calculate_offset(void *address){
  uint64_t mmap_offset = (uint64_t)address - (uint64_t)mmap_address;
  return mmap_offset;
}

int
check_flag()
{
	return non_checkpoint_flag;
}

void
shift_to_left(struct node *found_node, int filepos)
{
	non_checkpoint_flag = 1;
	if (flog == NULL) {
		return;
	}
	for (int i = 0; i < MAX_VERSIONS - 1; i++) {
		found_node->c_data.data[i] = checkpoint_file_curr[filepos];
		checkpoint_file_curr[filepos] = (void *)((uint64_t)checkpoint_file_curr[filepos] +
						found_node->c_data.size[i + 1]);
		memcpy(found_node->c_data.data[i],
		       found_node->c_data.data[i + 1],
		       found_node->c_data.size[i + 1]);
		found_node->c_data.size[i] = found_node->c_data.size[i + 1];
		found_node->c_data.sequence_number[i] =
			found_node->c_data.sequence_number[i + 1];
	}
	non_checkpoint_flag = 0;
}

void
insert_value(const void *address, size_t size, const void *data_address,
	     uint64_t offset, int tx_id, void *addr)
{
	non_checkpoint_flag = 1;
	 printf("INSERT VALUE value of size %ld offset is %ld seq num is %d addr is %p pop is %p\n",
	          size, offset, sequence_number, address, addr);
	if (flog == NULL) {
		return;
	}
	int pos;
        struct file_node * foundfileNode = fileLookup((uint64_t)addr);
        if(foundfileNode != NULL)
          pos = foundfileNode->index;
        else{
          printf("pos is incorrect\n");
	  return;
        }
	// Look for address in hashmap
	struct node *found_node = lookup(offset, pos);
	struct checkpoint_data insert_data;
        //printf("after lookup pos is %d\n", pos);
	if (found_node == NULL) {
		// We need to insert node for address
		c_log[pos]->variable_count = c_log[pos]->variable_count + 1;
		variable_count = variable_count + 1;
		insert_data.address = address;
		insert_data.offset = offset;
		insert_data.size[0] = size;
		insert_data.version = 0;
		insert_data.tx_id[0] = tx_id;
		insert_data.sequence_number[0] = sequence_number;
		__atomic_fetch_add(&sequence_number, 1, __ATOMIC_SEQ_CST);
		insert_data.data[0] = checkpoint_file_curr[pos];
		// pthread_mutex_lock(&mutex);
		checkpoint_file_curr[pos] =
			(void *)((uint64_t)checkpoint_file_curr[pos] + size);
		memcpy(insert_data.data[0], data_address, size);
		// pthread_mutex_unlock(&mutex);
		insert(offset, insert_data, pos);
	} else if (found_node->c_data.data_type == -1) {
		c_log[pos]->variable_count = c_log[pos]->variable_count + 1;
		variable_count = variable_count + 1;
		insert_data.address = address;
		insert_data.offset = offset;
		insert_data.size[0] = size;
		insert_data.version = 0;
		insert_data.tx_id[0] = tx_id;
		insert_data.sequence_number[0] = sequence_number;
		__atomic_fetch_add(&sequence_number, 1, __ATOMIC_SEQ_CST);
		insert_data.data[0] = checkpoint_file_curr[pos];
		// pthread_mutex_lock(&mutex);
		checkpoint_file_curr[pos] =
			(void *)((uint64_t)checkpoint_file_curr[pos] + size);
		memcpy(insert_data.data[0], data_address, size);
		// pthread_mutex_unlock(&mutex);
		insert(offset, insert_data, pos);
	} else {
		if (found_node->c_data.version + 1 == MAX_VERSIONS) {
			// shift_to_left(found_node, pos);
		} else {
			found_node->c_data.version += 1;
		}
		int data_index = found_node->c_data.version;
		found_node->c_data.address = address;
		found_node->c_data.size[data_index] = size;
		found_node->c_data.data[data_index] = checkpoint_file_curr[pos];
		found_node->c_data.tx_id[data_index] = tx_id;
		// pthread_mutex_lock(&mutex);
		checkpoint_file_curr[pos] =
			(void *)((uint64_t)checkpoint_file_curr[pos] + size);
		memcpy(found_node->c_data.data[data_index], data_address, size);
		// pthread_mutex_unlock(&mutex);
		found_node->c_data.sequence_number[data_index] =
			sequence_number;
		__atomic_fetch_add(&sequence_number, 1, __ATOMIC_SEQ_CST);
	}
	non_checkpoint_flag = 0;
	print_checkpoint_log(pos);
}

void
checkpoint_free(uint64_t off)
{
	// printf("off %ld\n", off);
	struct node *temp = lookup(off, 0);
	if (!temp)
		return;
	temp->c_data.free_flag = 1;
}

int
hashCode(uint64_t offset, int filepos)
{
	int ret_val;
	if (offset < 0) {
		ret_val = (int)(offset % c_log[filepos]->size);
		ret_val = -ret_val;
	}
	ret_val = (int)(offset % c_log[filepos]->size);
	return ret_val;
}

int fileHash(uint64_t address)
{
  int ret_val;
  if(address < 0){
    ret_val = (int)(address % PM_FILES);
    ret_val = -ret_val;
  }
  ret_val = (int)(address % PM_FILES);
  return ret_val;
}



void
insert(uint64_t offset, struct checkpoint_data c_data, int filepos)
{
	int pos = hashCode(offset, filepos);
	struct node *list = c_log[filepos]->list[pos];
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
	//   pthread_mutex_lock(&mutex);
	struct node *newNode = (struct node *)checkpoint_file_curr[filepos];
	checkpoint_file_curr[filepos] =
		(void *)((uint64_t)checkpoint_file_curr[filepos] + sizeof(struct node));
	//   pthread_mutex_unlock(&mutex);
	newNode->c_data = c_data;
	newNode->next = list;
	newNode->offset = offset;
	c_log[filepos]->list[pos] = newNode;
}

void fileInsert(uint64_t address, int val){
  int pos = fileHash(address);
  struct file_node *list = flog->list[pos];
  struct file_node *temp = list;
  while(temp){
    if(temp->address == address){
      temp->index = val;
      return;
    }
    temp = temp->next;
  }
  struct file_node *newNode = (struct file_node *)malloc(sizeof(struct file_node));
  newNode->index = val;
  newNode->next = list;
  newNode->address = address;
  flog->list[pos] = newNode;
  //list = flog->list[pos];
}

struct node *
lookup(uint64_t offset, int filepos)
{
	int pos = hashCode(offset, filepos);
	struct node *list = c_log[filepos]->list[pos];
	struct node *temp = list;
	while (temp) {
		if (temp->offset == offset) {
			return temp;
		}
		temp = temp->next;
	}
	return NULL;
}

struct file_node *fileLookup(uint64_t address){
  int pos = fileHash(address);
  struct file_node *list = flog->list[pos];
  struct file_node *temp = list;
  while(temp){
    if (temp->address == address){
      return temp;
    }
    temp = temp->next;
  }
  return NULL;
}

bool
check_pmem(void *addr, size_t size)
{
	uint64_t address = (uint64_t)addr;
	uint64_t pmem_base = (uint64_t)checkpoint_file_address;
	printf("address is %ld, pmem_base is %ld, mapped is %ld\n", address,
	       pmem_base, mapped_len);
	if (address >= pmem_base && address <= (pmem_base + mapped_len)) {
		return true;
	}
	return false;
}

bool
check_offset(uint64_t offset)
{
	if (offset < mapped_len)
		return true;
	return false;
}

void
print_checkpoint_log(int pos)
{
	printf("**************\n\n");
	struct node *list;
	struct node *temp;
	printf("c_log is %p\n", c_log[pos]);
	printf("c_log->list is %p\n", c_log[pos]->list);
	for (int i = 0; i < (int)c_log[pos]->size; i++) {
		list = c_log[pos]->list[i];
		temp = list;
		while (temp) {
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
				printf("version is %d size is %ld seq num is %d value is %f or %d or %s\n",
				       j, temp->c_data.size[j],
				       temp->c_data.sequence_number[j],
				       *((double *)temp->c_data.data[j]),
				       *((int *)temp->c_data.data[j]),
				       (char *)temp->c_data.data[j]);
				printf("tx pointer is %d\n",
				       temp->c_data.tx_id[j]);
			}
			temp = temp->next;
		}
	}
	printf("the variable count is %d %d\n", variable_count,
	       c_log[pos]->variable_count);
}
