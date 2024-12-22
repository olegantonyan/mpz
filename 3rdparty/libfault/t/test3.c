#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

void
h()
{
  printf("h(): crashing\n");
  fflush(stdout);

  assert(false);
}

void
g()
{
  printf("g(): gonna crash soon\n");
  h();
}

void
f()
{
  printf("f(): presumably LD_PRELOADed\n");
  g();
}

int
main(int ac, char** av)
{
  (void)ac, (void)av;

  f(); /* Go */
  return 0;
}
