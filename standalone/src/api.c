#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <omp.h>

#include "uthash.h"
#include "xmem.h"

static char xmem_fname_pattern[XMEM_MAX_PATH_LEN] = "XXXXXX";
static char xmem_fname_path[XMEM_MAX_PATH_LEN] = "/tmp";

char xmem_fname_template[XMEM_MAX_PATH_LEN] = "/tmp/fm_XXXXXX";
size_t xmem_threshold = 2000000000;

int xmem_advise = MADV_SEQUENTIAL;
int xmem_offset = 0;

/* The next functions allow applications to inspect and change default
 * settings. The application must dynamically locate them with dlsym after
 * the library is loaded. They represent the xmem API, such as it is.
 *
 * API functions defined below include:
 * size_t xmem_set_threshold (size_t j)
 * int xmem_set_template (char *template)
 * int xmem_set_pattern (char *pattern)
 * int xmem_set_path (char *path)
 * int xmem_madvise (int j)
 * int xmem_memcpy_offset (int j)
 * char * xmem_lookup(void *addr)
 * char * xmem_get_template()
 */

/* Set and get threshold size.
 * INPUT
 * j: proposed new xmem_threshold size
 * OUTPUT
 * (return value): xmem_threshold size on exit
 */
size_t
xmem_set_threshold (size_t j)
{
  if (j > 0)
  {
    omp_set_nest_lock (&lock);
    xmem_threshold = j;
    omp_unset_nest_lock (&lock);
  }
  return xmem_threshold;
}

/* Set madvise option */
int
xmem_madvise (int j)
{
  if(j> -1)
  {
    omp_set_nest_lock (&lock);
    xmem_advise = j;
    omp_unset_nest_lock (&lock);
  }
  return xmem_advise;
}

/* Set memcpy offset option */
int
xmem_memcpy_offset (int j)
{
  if(j> -1)
  {
    omp_set_nest_lock (&lock);
    xmem_offset = j;
    omp_unset_nest_lock (&lock);
  }
  return xmem_offset;
}

/* Set the file template character string
 * INPUT name, a proposed new xmem_fname_template string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
xmem_set_template (char *name)
{
  char *s;
  int n = strlen(name);
  if(n<6) return -1;
/* Validate mkostemp file name */
  s = name + (n-6);
  if(strncmp(s, "XXXXXX", 6)!=0) return -2;
  omp_set_nest_lock (&lock);
  memset(xmem_fname_template, 0, XMEM_MAX_PATH_LEN);
  strncpy(xmem_fname_template, name, XMEM_MAX_PATH_LEN);
  omp_unset_nest_lock (&lock);
  return 0;
}
/* Set the file pattern character string
 * INPUT name, a proposed new xmem_fname_pattern string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
xmem_set_pattern (char *name)
{
  char *s;
  int n = strlen(name);
  if(n<6) return -1;
/* Validate mkostemp file name */
  s = name + (n-6);
  if(strncmp(s, "XXXXXX", 6)!=0) return -2;
  omp_set_nest_lock (&lock);
  memset(xmem_fname_template, 0, XMEM_MAX_PATH_LEN);
  memset(xmem_fname_pattern, 0, XMEM_MAX_PATH_LEN);
  strncpy (xmem_fname_pattern, name, XMEM_MAX_PATH_LEN);
  snprintf(xmem_fname_template, XMEM_MAX_PATH_LEN, "%s/%s",
           xmem_fname_path, xmem_fname_pattern);
  omp_unset_nest_lock (&lock);
  return 0;
}
/* Set the file directory path character string
 * INPUT name, a proposed new xmem_fname_path string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
xmem_set_path (char *p)
{
  int n = strlen(p);
  if(n<6) return -1;
  omp_set_nest_lock (&lock);
  memset(xmem_fname_template, 0, XMEM_MAX_PATH_LEN);
  memset(xmem_fname_path, 0, XMEM_MAX_PATH_LEN);
  strncpy (xmem_fname_path, p, XMEM_MAX_PATH_LEN);
  snprintf(xmem_fname_template, XMEM_MAX_PATH_LEN, "%s/%s",
           xmem_fname_path, xmem_fname_pattern);
  omp_unset_nest_lock (&lock);
  return 0;
}
/* Return a copy of the xmem_fname_template (allocated internally...
 * it is up to the caller to free the returned copy!! Yikes! My rationale
 * is this--how could I trust a buffer provided by the caller?)
 */
char *
xmem_get_template()
{
  char *s;
  omp_set_nest_lock (&lock);
  s = strndup(xmem_fname_template,XMEM_MAX_PATH_LEN);
  omp_unset_nest_lock (&lock);
  return s;
}
/* Lookup an address, returning NULL if the address is not found or a strdup
 * locally-allocated copy of the backing file path for the address. No guarantee
 * is made that the address or backing file will be valid after this call, so
 * it's really up to the caller to make sure free is not called on the address
 * simultaneously with this call. CALLER'S RESPONSIBILITY TO FREE RESULT!
 */
char *
xmem_lookup(void *addr)
{
  char *f = NULL;
  struct map *x;
  omp_set_nest_lock (&lock);
  HASH_FIND_PTR (flexmap, &addr, x);
  if(x) f = strndup(x->path,XMEM_MAX_PATH_LEN);
  omp_unset_nest_lock (&lock);
  return f;
}
// XXX Also add a list all mappings function??
