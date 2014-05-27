#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <omp.h>

#include "uthash.h"
#include "flexmem.h"

static char flexmem_fname_pattern[FLEXMEM_MAX_PATH_LEN] = "XXXXXX";
static char flexmem_fname_path[FLEXMEM_MAX_PATH_LEN] = "/tmp";

char flexmem_fname_template[FLEXMEM_MAX_PATH_LEN] = "/tmp/fm_XXXXXX";
size_t flexmem_threshold = 2000000000;

int flexmem_advise = MADV_SEQUENTIAL;

/* The next functions allow applications to inspect and change default
 * settings. The application must dynamically locate them with dlsym after
 * the library is loaded. They represent the flexmem API, such as it is.
 *
 * API functions defined below include:
 * size_t flexmem_set_threshold (size_t j)
 * int flexmem_set_template (char *template)
 * int flexmem_set_pattern (char *pattern)
 * int flexmem_set_path (char *path)
 * char * flexmem_lookup(void *addr)
 * char * flexmem_get_template()
 */

/* Set and get threshold size.
 * INPUT
 * j: proposed new flexmem_threshold size
 * OUTPUT
 * (return value): flexmem_threshold size on exit
 */
size_t
flexmem_set_threshold (size_t j)
{
  if (j > 0)
  {
    omp_set_nest_lock (&lock);
    flexmem_threshold = j;
    omp_unset_nest_lock (&lock);
  }
  return flexmem_threshold;
}

/* Set madvise option */
int
flexmem_madvise (int j)
{
  if(j> -1)
  {
    omp_set_nest_lock (&lock);
    flexmem_advise = j;
    omp_unset_nest_lock (&lock);
  }
  return flexmem_advise;
}

/* Set the file template character string
 * INPUT name, a proposed new flexmem_fname_template string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
flexmem_set_template (char *name)
{
  char *s;
  int n = strlen(name);
  if(n<6) return -1;
/* Validate mkostemp file name */
  s = name + (n-6);
  if(strncmp(s, "XXXXXX", 6)!=0) return -2;
  omp_set_nest_lock (&lock);
  memset(flexmem_fname_template, 0, FLEXMEM_MAX_PATH_LEN);
  strncpy(flexmem_fname_template, name, FLEXMEM_MAX_PATH_LEN);
  omp_unset_nest_lock (&lock);
  return 0;
}
/* Set the file pattern character string
 * INPUT name, a proposed new flexmem_fname_pattern string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
flexmem_set_pattern (char *name)
{
  char *s;
  int n = strlen(name);
  if(n<6) return -1;
/* Validate mkostemp file name */
  s = name + (n-6);
  if(strncmp(s, "XXXXXX", 6)!=0) return -2;
  omp_set_nest_lock (&lock);
  memset(flexmem_fname_template, 0, FLEXMEM_MAX_PATH_LEN);
  memset(flexmem_fname_pattern, 0, FLEXMEM_MAX_PATH_LEN);
  strncpy (flexmem_fname_pattern, name, FLEXMEM_MAX_PATH_LEN);
  snprintf(flexmem_fname_template, FLEXMEM_MAX_PATH_LEN, "%s/%s",
           flexmem_fname_path, flexmem_fname_pattern);
  omp_unset_nest_lock (&lock);
  return 0;
}
/* Set the file directory path character string
 * INPUT name, a proposed new flexmem_fname_path string
 * Returns 0 on sucess, a negative number otherwise.
 */
int
flexmem_set_path (char *p)
{
  int n = strlen(p);
  if(n<6) return -1;
  omp_set_nest_lock (&lock);
  memset(flexmem_fname_template, 0, FLEXMEM_MAX_PATH_LEN);
  memset(flexmem_fname_path, 0, FLEXMEM_MAX_PATH_LEN);
  strncpy (flexmem_fname_path, p, FLEXMEM_MAX_PATH_LEN);
  snprintf(flexmem_fname_template, FLEXMEM_MAX_PATH_LEN, "%s/%s",
           flexmem_fname_path, flexmem_fname_pattern);
  omp_unset_nest_lock (&lock);
  return 0;
}
/* Return a copy of the flexmem_fname_template (allocated internally...
 * it is up to the caller to free the returned copy!! Yikes! My rationale
 * is this--how could I trust a buffer provided by the caller?)
 */
char *
flexmem_get_template()
{
  char *s;
  omp_set_nest_lock (&lock);
  s = strndup(flexmem_fname_template,FLEXMEM_MAX_PATH_LEN);
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
flexmem_lookup(void *addr)
{
  char *f = NULL;
  struct map *x;
  omp_set_nest_lock (&lock);
  HASH_FIND_PTR (flexmap, &addr, x);
  if(x) f = strndup(x->path,FLEXMEM_MAX_PATH_LEN);
  omp_unset_nest_lock (&lock);
  return f;
}
// XXX Also add a list all mappings function??
