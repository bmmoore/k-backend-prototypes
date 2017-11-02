#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

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
    printf(opnames[base->op], base->a, base->b, base->c, base->immediate);
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

jmp_buf stuck_tgt;

int64_t aeval(struct node top) {
  switch(top.op) {
  case ACon:
    return top.immediate;
  case AVar:
    return vars[top.immediate];
  case Add:
    return aeval(permanent[top.a]) + aeval(permanent[top.b]);
  case Div:
  {
    int64_t n = aeval(permanent[top.a]);
    int64_t d = aeval(permanent[top.b]);
    if (d != 0) {
      return n/d;
    }
  }
  default:
    longjmp(stuck_tgt,1);
  }
}

uint64_t beval(struct node top) {
  switch(top.op) {
    case BCon:
      return top.immediate;
    case Not:
      return !beval(permanent[top.a]);
    case And:
      return beval(permanent[top.a]) && beval(permanent[top.b]);
    case Le:
      return aeval(permanent[top.a]) <= aeval(permanent[top.b]);
    default:
      longjmp(stuck_tgt,1);
  }
}

void exec(struct node top) {
  switch(top.op) {
  case Skip:
    break; 
  case Seq:
    exec(permanent[top.a]);
    exec(permanent[top.b]);
    break;
  case If:
    if (beval(permanent[top.a])) {
      exec(permanent[top.b]);
    } else {
      exec(permanent[top.c]);
    }
    break;
  case While:
  {
    struct node condition = permanent[top.a];
    struct node body = permanent[top.b];
    while (beval(condition)) {
      exec(body);
    }
    break;
  }
  case Assign:
  {
    int64_t x = aeval(permanent[top.a]);
    vars[top.immediate] = x;
    break;
  }
  }
}

void run_k(struct node top) {
  struct node varList = permanent[top.a];
  struct node body = permanent[top.b];
  while (varList.op != Nil) {
    int64_t v = permanent[varList.a].immediate; 
    vars[v] = 0;
    varList = permanent[varList.b];
  }
  if (!setjmp(stuck_tgt)) {
    exec(body);
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
  // dump_seg("[%2d] = ",permanent, permanent_next, "\n");
  run_k(pgm);
  printf("Done. n=%"PRIi64" sum=%"PRIi64"\n",vars[0],vars[1]);
  return 0;
}
