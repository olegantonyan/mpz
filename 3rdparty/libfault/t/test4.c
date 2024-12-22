#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "libfault.h"

void
h()
{
  printf("h(): not crashing\n");
  fflush(stdout);

  assert(true);
}

void
g()
{
  printf("g(): doing fine\n");
  h();
}

void
f()
{
  printf("f(): hey there\n");
  g();
}

int
main(int ac, char** av)
{
  (void)ac, (void)av;

  libfault_init();
  libfault_set_app_name("Testing application #4");
  libfault_set_app_version("0.0");
  libfault_set_log_name("/tmp/libfault-test4.");
  libfault_set_bugreport_url("https://foo.com/bugs.cgi?report=new");
  libfault_install_handlers();

  f(); /* Go */
  return 0;
}
