#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* _mallocInit(int type_size, int size, void* init_list) {
  void* mem = malloc(size * type_size);

  if (mem == NULL) return mem;

  memcpy(mem, init_list, type_size * size);

  return mem;
}

#define NUMARGS(actualType, ...)  (sizeof((actualType[]){__VA_ARGS__})/sizeof(actualType))

#define mallocInit(actualType, ...)\
  (actualType*)_mallocInit(sizeof(actualType), NUMARGS(actualType, __VA_ARGS__), (actualType[]){__VA_ARGS__})

typedef struct {
  uint32_t id;
  char* smt;
} foo_t;

int main() {
  // works if .smt=NULL is replaced by .smt=<any number>
  foo_t *l = mallocInit(foo_t,{ .id=1, .smt=mallocInit(char, "opa") }, { .id=10, .smt=1 });
  printf("%s %s\n", l[0].smt, l[1].smt );
  free(l); // leaking memory, deal with it
}

