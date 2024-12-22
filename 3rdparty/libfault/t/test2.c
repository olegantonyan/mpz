#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "libfault.h"

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
  printf("f(): libfault setup done\n");
  g();
}

int
main(int ac, char** av)
{
  (void)ac, (void)av;

  libfault_init();
  libfault_set_app_name("Testing application #2");
  libfault_set_app_version("0.0");
  libfault_set_log_name("/tmp/libfault-test2.");
  libfault_set_bugreport_url("https://foo.com/bugs.cgi?report=new");
  libfault_install_handlers();

  f(); /* Go */
  return 0;
}
