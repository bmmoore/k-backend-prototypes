#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG 1

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

struct node mkNullary(uint32_t opcode) {
  struct node n;
  n.op = opcode;
  return n;
}
struct node mkImm(uint32_t opcode, uint64_t imm) {
  struct node n;
  n.op = opcode;
  n.immediate = imm;
  return n;
}
struct node mkUnary(uint32_t opcode, uint32_t a) {
  struct node n;
  n.op = opcode;
  n.a = a;
  return n;
}
struct node mkUnaryImm(uint32_t opcode, uint32_t a, uint64_t imm) {
  struct node n;
  n.op = opcode;
  n.a = a;
  n.immediate = imm;
  return n;
}
struct node mkBinary(uint32_t opcode, uint32_t a, uint32_t b) {
  struct node n;
  n.op = opcode;
  n.a = a;
  n.b = b;
  return n;
}
struct node mkTernary(uint32_t opcode, uint32_t a, uint32_t b, uint32_t c) {
  struct node n;
  n.op = opcode;
  n.a = a;
  n.b = b;
  n.c = c;
  return n;
}
