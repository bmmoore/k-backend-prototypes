#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

// 16 bytes. Good.
struct node {
  uint32_t op;
  int64_t a;
  int64_t b;
};

struct node mkUnary(uint32_t opcode, uint64_t a) {
  struct node n;
  n.op = opcode;
  n.a = a;
  return n;
}
struct node mkBinary(uint32_t opcode, uint64_t a, uint64_t b) {
  struct node n;
  n.op = opcode;
  n.a = a;
  n.b = b;
  return n;
}

enum OpCode {
  Sum = 0,
  Loop = 1,
  Done = 2,
};

const char* opnames[64] =
  {
    [0]      = "Sum %1$ld",
    [1]      = "Loop %1$ld %2$ld",
    [2]      = "Done %4$ld",
  };

void dump_seg(const char* prefix, struct node* base, struct node* end, const char* suffix) {
  int ix = 0;
  while (base < end) {
    printf(prefix, ix);
    // printf(opnames[base->op], base->a, base->b, base->c, base->immediate);
    printf(opnames[base->op], base->a, base->b);
    printf("%s", suffix);
    ++ix;
    ++base;
  }
}

struct node* stack;
struct node* stack_base;
struct node* stack_top;

struct node* permanent;
struct node* permanent_top;
struct node* permanent_next;

struct node* heap_base;
struct node* heap_top;
struct node* heap;

void initGC() {
  stack_base = aligned_alloc(0x1000000,0x1000000); // 1MB stack
  if (!stack_base) {
    exit(1);
  }
  stack_top = stack_base + 0x100000;
  stack = stack_top;

  permanent = aligned_alloc(0x10,2048*sizeof(struct node));
  permanent_top = permanent + 2048;
  permanent_next = permanent;
}

void push_node(struct node n) {
  *--stack = n;
}

struct node run_k(struct node top) {
  int64_t n_val, sum_val;
 pgm:
  {
    switch(top.op) {
    case Sum:
      n_val = top.a;
      sum_val = 0;
      top.op = Loop;
      goto loop;
    }
  };
 loop:
  {
    if (n_val == 0) {
      top.op = Done;
      top.a = sum_val;
      goto done;
    } else {
      int64_t n1 = n_val-1;
      int64_t sum1 = sum_val+n_val;
      n_val = n1;
      sum_val = sum1;
      goto loop;
    }
  }
 done:
  return (struct node){Done,sum_val};
}
struct node load_sum(long n) {
  return (struct node){Sum,n,0};
}

int main(int argc, char** argv) {
  initGC();
  struct node pgm = load_sum(atoi(argv[1]));
  struct node result = run_k(pgm);
  printf("Done. sum=%"PRIi64"\n",result.a);
  return 0;
}





