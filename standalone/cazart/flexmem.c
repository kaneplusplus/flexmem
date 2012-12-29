#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <omp.h>

#include "uthash.h"
#define FLEXMEM_MAX_PATH_LEN 4096

#undef DEBUG
#define DEBUG



/* NOTES
 *
 * This is a hack that uses reasonably well-known methods to overload malloc,
 * free, calloc, and realloc such that allocations above a threshold are memory
 * mapped files. The library maintains a mapping of allocated addresses and
 * corresponding backing files and cleans up files as they are de-allocated.
 * The idea is to help programs allocate large objects out of core while
 * explicitly avoiding the system swap space. The library provides a crude API
 * that lets programs change the mapping file path and threshold value.
 *
 * We use a somewhat tricky hack to initialize the library.  We compile with
 * -pie to introduce a main function local to the library (the library is now
 * also directly executable). But our main function is just a stub used to
 * invoke the flexmem_init function marked with a GNU constructor attribute
 * when loaded. Thus, even if libflexmem.so is dynamically loaded from an
 * already running program, the flexmem_init function should get invoked. The
 * init should be the only non-reentrant part of the code that isn't inside a
 * mutex.
 *
 * The use of mutex-like locking below is probably not the most efficient
 * approach--a finer read/write locking mechanism might be better since uthash
 * supports threaded reading.  But, we like omp for its simplicity and
 * portability.
 *
 * XXX Instead of RTLD_NEXT, we should maybe explicitly locate the default
 * symbols in libc. For example, if we called this init routine from within
 * another dyanmic library, the default symbol lookup might fail, right?
 * Perhaps we could use the non-POSIX dladdr function? Is that reasonably
 * portable?
 *
 * Should we have used the GNU libc hooks instead? We knew you would ask, and
 * we do abuse them in our Frankenstein calloc function. Generally though, the
 * hooks don't work so well in our application.
 *
 * Be cautious when debugging about placement of printf, write, etc. These
 * things often end up re-entering one of our functions.
 */

static void flexmem_init (void) __attribute__ ((constructor));
static void *(*flexmem_hook) (size_t, const void *);

char flexmem_fname_template[FLEXMEM_MAX_PATH_LEN] = "/tmp/fm_XXXXXX";
size_t flexmem_threshold = 1000000;
volatile int READY = 0;

/* The map data structure stores information about a particular
 * mapping.
 */
struct map
{
  void *addr;                   /* Memory address, list key */
  char *path;                   /* File path */
  size_t length;                /* Mapping length */
  UT_hash_handle hh;            /* Make this thing uthash-hashable */
};

/* The global variable flexmap is a key-value list of addresses (keys)
 * and file paths (values).
 */
struct map *flexmap;
omp_nest_lock_t lock;
void *(*flexmem_global_malloc) (size_t);

/* Flexmem initialization
 *
 * Initializes synchronizing lock variable and address/file key/value map.
 *
 */
static void
flexmem_init ()
{
  omp_init_nest_lock (&lock);
  READY = 1;
  flexmem_hook = __malloc_hook;
}

// XXX What about a finalizer for when the library is unloaded?!?!?
// XXX Add this...


/* The next functions allow applications to inspect and change default
 * settings. The application must dynamically locate them with dlsym after the
 * library is loaded.
 * flexmem_set_threshold (INPUT/OUTPUT size_t *j)
 * flexmem_set_template (INPUT/OUTPUT char *name, INPUT int l--max length of name)
 * They represent the flexmem API, such as it is.
 */

// XXX lock all these things !
void
flexmem_set_threshold (size_t * j)
{
  if ((*j) > 0)
    flexmem_threshold = *j;
  *j = flexmem_threshold;
}

void
flexmem_set_template (char *name)
{
// XXX validate template
  if (name)
    strncpy (flexmem_fname_template, name, FLEXMEM_MAX_PATH_LEN);
}

// XXX add a flexmem_get_template ??
// XXX add a flexmem_get_file(void *address) function to look up a file
// XXX in the hash?

void
freemap (struct map *m)
{
  void *(*flexmem_default_free) (void *) =
    (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
  if (m)
    {
      if (m->path)
        (*flexmem_default_free) (m->path);
      (*flexmem_default_free) (m);
    }
}


void *
malloc (size_t size)
{
  struct map *m;
  void *x;
  int j;
  int fd;
  void *(*flexmem_default_malloc) (size_t);

  flexmem_default_malloc = (void *(*)(size_t)) dlsym (RTLD_NEXT, "malloc");
#ifdef DEBUG
  printf ("malloc\n");
#endif
  if (size > flexmem_threshold && READY)
    {
      m = (struct map *) ((*flexmem_default_malloc) (sizeof (struct map)));
//      m->path = (char *) ((*flexmem_default_calloc) (sizeof(char), FLEXMEM_MAX_PATH_LEN));
      m->path = (char *) ((*flexmem_default_malloc) (FLEXMEM_MAX_PATH_LEN));
      bzero (m->path, FLEXMEM_MAX_PATH_LEN);
// XXX THERE NEEDS TO BE A LOCK AROUND THE TEMPLATE ACCESS
      strncpy (m->path, flexmem_fname_template, FLEXMEM_MAX_PATH_LEN);
      m->length = size;
      fd = mkostemp (m->path, O_RDWR | O_CREAT);
      j = ftruncate (fd, m->length);
      if (j < 0)
        {
          close (fd);
          unlink (m->path);
          freemap (m);
          return NULL;
        }
      m->addr =
        mmap (NULL, m->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      x = m->addr;
      close (fd);
#ifdef DEBUG
      printf ("Flexmem malloc address %p, size %lu, file  %s\n", m->addr,
              (unsigned long int) m->length, m->path);
#endif
      omp_set_nest_lock (&lock);
      HASH_ADD_PTR (flexmap, addr, m);
#ifdef DEBUG
      printf ("count = %u\n", HASH_COUNT (flexmap));
#endif
      omp_unset_nest_lock (&lock);
    }
  else
    {
      x = (*flexmem_default_malloc) (size);
    }
  return x;
}

void
free (void *ptr)
{
  void *(*flexmem_default_free) (void *);
  struct map *m;
#ifdef DEBUG
  printf ("free\n");
#endif
  if (!ptr)
    return;
  if (READY)
    {
      omp_set_nest_lock (&lock);
      HASH_FIND_PTR (flexmap, &ptr, m);
      if (m)
        {
          munmap (ptr, m->length);
#ifdef DEBUG
          printf ("Flexmem unmap address %p of size %lu\n", ptr,
                  (unsigned long int) m->length);
#endif
          unlink (m->path);
          HASH_DEL (flexmap, m);
          freemap (m);
          omp_unset_nest_lock (&lock);
          return;
        }
      omp_unset_nest_lock (&lock);
    }
  flexmem_default_free = (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
  (*flexmem_default_free) (ptr);
}


void *
realloc (void *ptr, size_t size)
{
  struct map *m;
  int j, fd;
  void *x;
  void *(*flexmem_default_realloc) (void *, size_t);
#ifdef DEBUG
  printf ("realloc\n");
#endif

/* Handle two special realloc cases: */
  if (ptr == NULL)
    return malloc (size);
  if (size == 0)
    {
      free (ptr);
      return NULL;
    }

  x = NULL;
  flexmem_default_realloc =
    (void *(*)(void *, size_t)) dlsym (RTLD_NEXT, "realloc");
  if (READY)
    {
      omp_set_nest_lock (&lock);
      HASH_FIND_PTR (flexmap, &ptr, m);
      if (m)
        {
/* Remove the current file mapping, truncate the file, and return a new
 * file mapping to the truncated file.
 */
          HASH_DEL (flexmap, m);
          munmap (ptr, m->length);
          m->length = size;
          fd = open (m->path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
          if (fd < 0)
            goto bail;
          j = ftruncate (fd, m->length);
          if (j < 0)
            goto bail;
          m->addr =
            mmap (NULL, m->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
          HASH_ADD_PTR (flexmap, addr, m);
          x = m->addr;
          close (fd);
#ifdef DEBUG
          printf ("Flexmem realloc address %p size %lu\n", ptr,
                  (unsigned long int) m->length);
#endif
          omp_unset_nest_lock (&lock);
          return x;
        }
      omp_unset_nest_lock (&lock);
    }
  x = (*flexmem_default_realloc) (ptr, size);
  return x;

bail:
  close (fd);
  unlink (m->path);
  freemap (m);
  return NULL;
}

/* calloc is a special case. Unfortunately, dlsym ultimately calls calloc,
 * thus a direct interposition here results in an infinite loop...We created
 * a fake calloc that relies on malloc, and avoids looking it up by abusing the
 * malloc.h hooks library. We told you this was a hack!
 */
void *
calloc (size_t count, size_t size)
{
  void *x;
  size_t n = count * size;
  if (READY && n > flexmem_threshold)
    {
#ifdef DEBUG
      printf ("Flexmem calloc...handing off to flexmem malloc\n");
#endif
      return malloc (n);
    }
  x = flexmem_hook (n, NULL);
  memset (x, 0, n);
  return x;
}






/* We use this stub of a main function, local to the library, along with a
 * GNU constructor function to initialize the library.
 */
int
main ()
{
  return 0;
}
