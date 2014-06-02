#include <fcntl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>

void testfunc (void);

int
main (int argc, void **argv)
{
  int j;
  void *x;
  char *derror;
  const char *y = "Cazart!";
  size_t SIZE = 1000000;

  size_t (*set_threshold) (size_t);
  void *handle;
  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) return -1;
  dlerror ();
  set_threshold = (size_t (*)(size_t ))dlsym(handle, "xmem_set_threshold");
  if ((derror = dlerror ()) == NULL)  (*set_threshold) (SIZE);
  dlclose (handle);

  printf ("> malloc below threshold\n");
  x = malloc (SIZE - 1);
  memcpy (x, (const void *) y, strlen (y));
  printf ("> (press a key to continue)\n");
  getc (stdin);
  free (x);

  printf ("> malloc above threshold\n");
  x = malloc (SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  printf ("> (press a key to continue)\n");
  getc (stdin);
  free (x);


  printf ("> calloc below threshold\n");
  x = calloc (SIZE - 1, 1);
  memcpy (x, (const void *) y, strlen (y));
  printf ("> (press a key to continue)\n");
  getc (stdin);
  free (x);

  printf ("> calloc above threshold\n");
  x = calloc (SIZE + 1, 1);
  memcpy (x, (const void *) y, strlen (y));
  printf ("> (press a key to continue)\n");
  getc (stdin);
  free (x);


  printf ("> malloc + realloc below threshold\n");
  x = malloc (SIZE - 1);
  x = realloc (x, SIZE + 1);
  memcpy (x, (const void *) y, strlen (y));
  printf ("> (press a key to continue)\n");
  getc (stdin);
  free (x);

  printf ("> malloc + realloc above threshold\n");
  x = malloc (SIZE + 1);
  x = realloc (x, SIZE + 10);
  memcpy (x, (const void *) y, strlen (y));
  printf ("> (press a key to continue)\n");
  getc (stdin);
  free (x);


  return 0;
}
