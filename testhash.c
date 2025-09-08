// compile with `make testhash`
#include <stdio.h>

int hash(const char *key) {
  /* one tcl hash algorithm */
  unsigned int hash = 0;
  while (*key) {
    hash = (hash << 5) - hash + (unsigned int)*key;
    ++key;
  }
  printf("%u", hash);
  return 0;
}

int main(int argc, const char **argv) {
  if (argc != 2) {
    printf("USAGE: %s STRING\n", argv[0]);
    return 1;
  }
  return hash(argv[1]);
}
