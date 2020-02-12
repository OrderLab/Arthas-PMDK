#ifndef LIBPMEMOBJ_CHECKPOINT_H
#define LIBPMEMOBJ_CHECKPOINT_H 1

#define MAX_LOGS 1000

struct checkpoint_tx_log{
  struct  tx_undo_runtime *undo[MAX_LOGS];
  struct ravl *ranges;
  uint64_t cache_offset;
  int undo_num;
};

extern struct checkpoint_tx_log ctx_log;
extern int undo_num;

void init_checkpoint_tx_log(void);
void save_checkpoint_tx_log(struct tx_undo_runtime *undo);
void print_undo(int num);

#endif
