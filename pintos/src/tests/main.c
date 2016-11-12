#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"

int
main (int argc UNUSED, char *argv[]) 
{
  msg("1");
  test_name = argv[0];
  
  msg ("begin");
  random_init (0);
  test_main ();
  msg ("end");
  return 0;
}
