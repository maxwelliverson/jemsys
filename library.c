#include "library.h"

#include <stdio.h>

#if !defined(__has_attribute)
#define __has_attribute(...)
#endif

void hello(void) {
  printf("Hello, World!\n");
}
