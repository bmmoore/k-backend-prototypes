#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

// 16 bytes. Good.
struct node {
  uint32_t op;
  uint32_t a;
  union {
    struct {
      uint32_t b;
      uint32_t c;
    };
    int64_t immediate;
  };
};

extern struct node mkNullary(uint32_t opcode);
extern struct node mkImm(uint32_t opcode, uint64_t imm);
extern struct node mkUnary(uint32_t opcode, uint32_t a);
extern struct node mkUnaryImm(uint32_t opcode, uint32_t a, uint64_t imm);
extern struct node mkBinary(uint32_t opcode, uint32_t a, uint32_t b);
extern struct node mkTernary(uint32_t opcode, uint32_t a, uint32_t b, uint32_t c);

#define Op1(Ix) 16  +Ix
#define Op2(Ix) 16*2+Ix
#define Op3(Ix) 16*3+Ix

enum OpCode {
  // nullary
  ACon = 0,
  AVar = 1,
  BCon = 2,
  // nullary stack-only
  DivR = 3,
  AddR = 4,
  LeR = 5,
  NotF = 6,
  AssignR = 7,
  Skip = 8,
  Nil = 9,

  // unary
  Not = Op1(0),
  Assign = Op1(1),
  // unary stack-only
  DivL = Op1(2),
  AddL = Op1(3),
  LeL = Op1(4),
  AndL = Op1(5),
  Pgm = Op1(6),
  Ind = Op1(7),

  // binary
  Div = Op2(0),
  Add = Op2(1),
  Le = Op2(2),
  And = Op2(3),
  While = Op2(4),
  Seq = Op2(5),
  Cons = Op2(6),
  // stack only
  WhileC = Op2(7),
  IfC = Op2(8),

  // ternary
  If = Op3(0),
};

const char* opnames[64] =
  {
    [0]      = "ACon %4$ld",
    [1]      = "AVar v%4$ld",
    [2]      = "BCon %4$ld",
    [3]      = "DivR %4$ld",
    [4]      = "AddR %4$ld",
    [5]      = "LeR %4$ld",
    [6]      = "NotF",
    [7]      = "AssignR %4$ld",
    [8]      = "Skip",
    [9]      = "Nil",
    [Op1(0)] = "Not %d",
    [Op1(1)] = "Assign v%4$ld %1$d",
    [Op1(2)] = "DivL %d",
    [Op1(3)] = "AddL %d",
    [Op1(4)] = "LeL %d",
    [Op1(5)] = "AndL %d",
    [Op1(6)] = "Pgm %d %d",
    [Op1(7)] = "Ind %d",
    [Op2(0)] = "Div %d %d",
    [Op2(1)] = "Add %d %d",
    [Op2(2)] = "Le %d %d",
    [Op2(3)] = "And %d %d",
    [Op2(4)] = "While %d %d",
    [Op2(5)] = "Seq %d %d",
    [Op2(6)] = "Cons %d %d",
    [Op2(7)] = "WhileC %d %d",
    [Op2(8)] = "IfC %d %d",
    [Op3(0)] = "If %d %d %d",
  };

void dump_seg(const char* prefix, struct node* base, struct node* end, const char* suffix) {
  int ix = 0;
  while (base < end) {
    printf(prefix, ix);
    // printf(opnames[base->op], base->a, base->b, base->c, base->immediate);
    printf("%s %d %d %d", opnames[base->op], base->a, base->b, base->c);
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

int64_t vars[2];

void run_k(struct node top) {
  int64_t acon_val, bcon_val;
  int64_t assign_var;
  int opl, opr, op3;
 pgm:
  {
#ifdef DEBUG
dump_seg("pgm:top = ",&top, &top+1, "\n");
printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
#endif
    struct node vl = permanent[top.a];
    switch(vl.op) {
    case Cons:
      vars[permanent[vl.a].immediate] = 0;
      top = (struct node){Pgm,vl.b,top.b,0};
      goto pgm;
    case Nil:
      top = permanent[top.b];
      goto stmt;
    }
  };
 stmt:
  {
#ifdef DEBUG
dump_seg("stmt:top = ",&top, &top+1, "\n");
printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
#endif
    switch(top.op) {
    case Skip:
      goto next_stmt;
    case Assign:
      assign_var = top.immediate;
      opl = top.a;
      top = permanent[opl];
      goto assign;
    case Ind:
      top = heap[top.a];
      goto stmt;
    case While:
      push_node(mkBinary(WhileC,top.a,top.b));
      top = permanent[top.a];
      goto while_op;
    case Seq:
      *--stack = permanent[top.b];
      opl = top.a;
      top = permanent[opl];
      goto stmt;
    case If:
      opr = top.b;
      op3 = top.c;
      top = permanent[top.a];
      goto if_op;
    default:
      printf("Unknown label %d\n", top.op);
      exit(3);
    }
  }
 next_stmt:
  {
#ifdef DEBUG
printf("next_stmt\n");
printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
#endif
    if (stack < stack_top) {
      top = *stack++;
      goto stmt;
    } else {
      return;
    }
  }
 aexp:
#ifdef DEBUG
dump_seg("aexp:top = ",&top, &top+1, "\n");
printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
#endif
  if (top.op == ACon) {
    acon_val = top.immediate;
    goto acon;
  }
 aexp_nonval:
#ifdef DEBUG
dump_seg("aexp_nonval:top = ",&top, &top+1, "\n");
printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
#endif
  {
    switch(top.op) {
    case AVar:
      acon_val = vars[top.immediate];
      goto acon;
    case Div:
      opr = top.b;
      top = permanent[top.a];
      goto div;
    case Add:
      opr = top.b;
      top = permanent[top.a];
      goto add;
    }
  }
 bexp:
#ifdef DEBUG
dump_seg("bexp:top = ",&top, &top+1, "\n");
printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
#endif
  if (top.op == BCon) {
    bcon_val = top.immediate;
    goto bcon;
  }
 bexp_nonval:
#ifdef DEBUG
dump_seg("bexp_nonval:top = ",&top, &top+1, "\n");
printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
#endif
  {
    switch(top.op) {
    case Not:
      top = permanent[top.a];
      goto not;
    case Le:
      opr = top.b;
      top = permanent[top.a];
      goto le;
    case And:
      opr = top.b;
      top = permanent[top.a];
      goto and;
    }
  }
 acon:
  {
    //    printf("acon: acon_val = %ld\n", acon_val);
    //    printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
    switch(stack->op) {
    case DivR:
      if (acon_val == 0) {
        exit(2);
      } else {
        acon_val = stack->immediate / acon_val;
        ++stack;
      }
      goto acon;
    case AddR:
      acon_val = stack->immediate + acon_val;
      ++stack;
      goto acon;
    case LeR:
      bcon_val = stack->immediate <= acon_val;
      ++stack;
      goto bcon;
    case AssignR:
      vars[stack->immediate] = acon_val;
      ++stack;
      goto next_stmt;
    case DivL:
      top = permanent[stack->a];
      ++stack;
      goto div_r;
    case AddL:
      top = permanent[stack->a];
      ++stack;
      goto add_r;
    case LeL:
      top = permanent[stack->a];
      ++stack;
      goto le_r;
    }
  }
 bcon:
  {
    //    printf("bcon: bcon_val = %ld\n", bcon_val);
    //    printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
    switch(stack->op) {
    case NotF:
      bcon_val = !bcon_val;
      ++stack;
      goto bcon;
    case AndL:
      opr = stack->a;
      ++stack;
      goto and_exec;
    case WhileC:
      if (bcon_val) {
        stack->op = While;
        top = permanent[stack->b];
        goto stmt;
      } else {
        ++stack;
        goto next_stmt;
      }
    case IfC:
      if(bcon_val) {
        top = permanent[stack->a];
      } else {
        top = permanent[stack->b];
      }
      ++stack;
      goto stmt;
    }
  }
 not:
  {
    if (top.op == BCon) {
      bcon_val = !top.immediate;
      goto bcon;
    } else {
      push_node(mkNullary(NotF));
      goto bexp_nonval;
    }
  }
 div:
  {
    int r = top.b;
    top = permanent[top.a];
    if (top.op == ACon) {
      opl = top.immediate;
      top = permanent[r];
      goto div_r;
    } else {
      push_node(mkUnary(DivL,r));
      goto aexp_nonval;
    }
  }
 div_r:
  {
    if (top.op == ACon) {
      if (top.immediate == 0) {
        exit(2);
      } else {
        acon_val = acon_val / top.immediate;
        goto acon;
      }
    } else {
      *--stack = (struct node){DivR,0,.immediate=acon_val};
      goto aexp_nonval;
    }
  }
 add: // left arg loaded in top, right index in opr
  {
    if (top.op == ACon) {
      acon_val = top.immediate;
      top = permanent[opr];
      goto add_r;
    } else {
      push_node(mkUnary(AddL,opr));
      goto aexp_nonval;
    }
  }
 add_r:
  {
    if (top.op == ACon) {
      acon_val = acon_val + top.immediate;
      goto acon;
    } else {
      push_node(mkImm(AddR,acon_val));
      goto aexp_nonval;
    }
  }
 le:
  //  dump_seg("le:top = ",&top, &top+1, "\n");
  //  printf("  opr = %d\n", opr);
  //  printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
  {
    if (top.op == ACon) {
      acon_val = top.immediate;
      top = permanent[opr];
      goto le_r;
    } else {
      push_node(mkUnary(LeL,opr));
      goto aexp_nonval;
    }
  }
 le_r:
  //  printf("le_r: acon_val = %ld\n", acon_val);
  //  dump_seg("top = ",&top, &top+1, "\n");
  //  printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
  {
    if(top.op == ACon) {
      bcon_val = acon_val <= top.immediate;
      goto bcon;
    } else {
      *--stack = (struct node){LeR,0,.immediate = acon_val};
      goto aexp_nonval;
    }
  }
 and:
  {
    opr = top.b;
    top = permanent[top.a];
    if (top.op == BCon) {
      bcon_val = top.immediate;
      goto and_exec;
    } else {
      push_node(mkUnary(LeL,opr));
      goto bexp_nonval;
    }
  }
 and_exec:
  {
    if (bcon_val) {
      top = permanent[opr];
      goto bexp;
    } else {
      goto bcon;
    }
  }
 while_op:
  {
#ifdef DEBUG
dump_seg("while_op:top = ",&top, &top+1, "\n");
printf("Stack: ");dump_seg("",stack, stack_top, " ~> ");puts("");
#endif
    if (top.op == BCon) {
      if (top.immediate) {
        stack->op = While;
        top = permanent[stack->b];
        goto stmt;
      } else {
        ++stack;
        goto next_stmt;
      }
    } else {
      goto bexp_nonval;
    }
  }
 if_op:
  {
    opr = top.b;
    op3 = top.c;
    top = permanent[top.a];
    if (top.op == BCon) {
      if (top.immediate) {
        top = permanent[opr];
      } else {
        top = permanent[op3];
      }
      goto stmt;
    } else {
      push_node(mkBinary(IfC,opr,op3));
      goto bexp_nonval;
    }
  }
 assign:
#ifdef DEBUG
dump_seg("assign:",&top, &top+1, "\n");
printf("stack: ");
dump_seg("",stack,stack_top," ~> ");
printf("\n");
#endif
  if (top.op == ACon) {
    vars[assign_var] = top.immediate;
    goto next_stmt;
  } else {
    push_node(mkImm(AssignR,assign_var));
    goto aexp_nonval;
  }
}

int perm(struct node n) {
  *permanent_next = n;
  return permanent_next++ - permanent;
}

int pCons(int l,int r) {
  return perm(mkBinary(Cons,l,r));
}
int pNil() {
  return perm(mkNullary(Nil));
}
int pVar(uint64_t id) {
  return perm(mkImm(AVar,id));
}
int pCon(uint64_t val) {
  return perm(mkImm(ACon,val));
}
int pSeq(int l, int r) {
  return perm(mkBinary(Seq,l,r));
}
int pAssign(int exp, uint64_t id) {
  return perm(mkUnaryImm(Assign,exp,id));
}
int pWhile(int cond, int body) {
  return perm(mkBinary(While,cond,body));
}
int pNot(int a) {
  return perm(mkUnary(Not,a));
}
int pAdd(int a, int b) {
  return perm(mkBinary(Add,a,b));
}
int pLe(int a, int b) {
  return perm(mkBinary(Le,a,b));
}

struct node load_sum(long n) {
  int vars = pCons(pVar(0),pCons(pVar(1),pNil()));
  int body =
    pSeq(pAssign(pCon(n),0),
    pSeq(pAssign(pCon(0),1),
         pWhile(pNot(pLe(pVar(0),pCon(0))),
                pSeq(pAssign(pAdd(pVar(1),pVar(0)),1),
                     pAssign(pAdd(pVar(0),pCon((uint64_t)-1)),0)))));
  return (struct node){Pgm,vars,body,0};
}

struct node load_test(long n) {
  int vars = pCons(pVar(0),pCons(pVar(1),pNil()));
  int body =
    pSeq(pAssign(pCon(n),0),
         pAssign(pCon(0),1));
  return (struct node){Pgm,vars,body,0};
}

int main(int argc, char** argv) {
  initGC();
  struct node pgm = load_sum(atoi(argv[1]));
#ifdef DEBUG
  dump_seg("[%2d] = ",permanent, permanent_next, "\n");
#endif
  run_k(pgm);
  printf("Done. n=%"PRIi64" sum=%"PRIi64"\n",vars[0],vars[1]);
  return 0;
}





