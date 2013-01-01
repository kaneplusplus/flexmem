#include <omp.h>
#include "uthash.h"

#define FLEXMEM_MAX_PATH_LEN 4096
#undef DEBUG
#define DEBUG2


/* NOTES
 *
 * Flexmem is a hack that uses well-known methods to overload various memory
 * allocation functions such that allocations above a threshold use memory
 * mapped files. The library maintains a mapping of allocated addresses and
 * corresponding backing files and cleans up files as they are de-allocated.
 * The idea is to help programs allocate large objects out of core while
 * explicitly avoiding the system swap space. The library provides a basic API
 * that lets programs change the mapping file path and threshold value.
 *
 * Be cautious when debugging about placement of printf, write, etc. These
 * things often end up re-entering one of our functions. The general rule here
 * is to keep things as minimal as possible.
 */

/* The map structure tracks the file mappings.  */
struct map
{
  void *addr;                   /* Memory address, list key */
  char *path;                   /* File path */
  size_t length;                /* Mapping length */
  pid_t pid;                    /* Process ID of owner (for fork) */
  UT_hash_handle hh;            /* Make this thing uthash-hashable */
};



/* These values can be changed using the crude API defined below. */
static char flexmem_fname_template[FLEXMEM_MAX_PATH_LEN] = "/tmp/fm_XXXXXX";
static size_t flexmem_threshold = 2000000000;

/* The global variable flexmap is a key-value list of addresses (keys)
 * and file paths (values).
 */
struct map *flexmap;
omp_nest_lock_t lock;
