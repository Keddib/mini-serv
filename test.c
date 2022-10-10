#include <stdio.h>

int main(int argc, char **argv) {
  if (argc < 2)
    return 1;
  char str[1024];
  sprintf(str, "string %d, : %s", 10, argv[1]);
  printf("new s : %s\n", str);
  return 0;
}
