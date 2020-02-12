#include "queue.h"
#include "ravl.h"
#include "obj.h"
#include "out.h"
#include "pmalloc.h"
#include "checkpoint.h"
#include "tx.h"

struct checkpoint_tx_log ctx_log;
int undo_num = 0;

void init_checkpoint_tx_log(){
  ctx_log.undo_num = 1;
}

void save_checkpoint_tx_log(struct tx_undo_runtime *undo){
  ctx_log.undo[undo_num] = undo;
  undo_num++;
}

void print_undo(int num){
  
}
