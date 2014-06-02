/*   ___    ___ _____ ______   _______   _____ ______      
 *  |\  \  /  /|\   _ \  _   \|\  ___ \ |\   _ \  _   \
 *  \ \  \/  / | \  \\\__\ \  \ \   __/|\ \  \\\__\ \  \
 *   \ \    / / \ \  \\|__| \  \ \  \_|/_\ \  \\|__| \  \
 *    /     \/   \ \  \    \ \  \ \  \_|\ \ \  \    \ \  \
 *   /  /\   \    \ \__\    \ \__\ \_______\ \__\    \ \__\
 *  /__/ /\ __\    \|__|     \|__|\|_______|\|__|     \|__|
 *  |__|/ \|__|                                            
 */                                                        
#include <omp.h>
#include "uthash.h"

#define XMEM_MAX_PATH_LEN 4096
#undef DEBUG
#undef DEBUG2

#define DEBUG2


/* NOTES
 *
 * Xmem uses well-known methods to overload various memory
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


/* These global values can be changed using the basic API defined in api.c. */
extern char xmem_fname_template[];
extern size_t xmem_threshold;
extern int xmem_advise;

/* The xmem_offset global can be set by the api. It affects memcpy by
 * searching for keys offset from the given memcpy address.
 */
extern int xmem_offset;

/* The global variable flexmap is a key-value list of addresses (keys) and file
 * paths (values). The recursive OpenMP lock is used widely in the library and
 * API functions.
 */
struct map *flexmap;
omp_nest_lock_t lock;
