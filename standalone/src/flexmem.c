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
#undef DEBUG2


/* NOTES
 *
 * Flexmem is a hack that uses reasonably well-known methods to overload malloc,
 * free, calloc, and realloc such that allocations above a threshold are memory
 * mapped files. The library maintains a mapping of allocated addresses and
 * corresponding backing files and cleans up files as they are de-allocated.
 * The idea is to help programs allocate large objects out of core while
 * explicitly avoiding the system swap space. The library provides a crude API
 * that lets programs change the mapping file path and threshold value.
 *
 * The use of mutex-like locking below is probably not the most efficient
 * approach--a finer read/write locking mechanism might be better since uthash
 * supports threaded reading.  But, we like omp for its simplicity and
 * portability.
 *
 * Be cautious when debugging about placement of printf, write, etc. These
 * things often end up re-entering one of our functions.
 */

/* The map structure tracks the file mappings.  */
struct map
{
  void *addr;                   /* Memory address, list key */
  char *path;                   /* File path */
  size_t length;                /* Mapping length */
  UT_hash_handle hh;            /* Make this thing uthash-hashable */
};

static void flexmem_init (void) __attribute__ ((constructor));
static void flexmem_finalize (void) __attribute__ ((destructor));
static void *(*flexmem_hook) (size_t, const void *);
static void *(*flexmem_default_free) (void *);
static void *(*flexmem_default_malloc) (size_t);
static void *(*flexmem_default_realloc) (void *, size_t);
void freemap (struct map *);
static int READY = 0;

/* These values can be changed using the crude API defined below. */
static char flexmem_fname_template[FLEXMEM_MAX_PATH_LEN] = "/tmp/fm_XXXXXX";
static char flexmem_fname_pattern[FLEXMEM_MAX_PATH_LEN] = "XXXXXX";
static char flexmem_fname_path[FLEXMEM_MAX_PATH_LEN] = "/tmp";
static size_t flexmem_threshold = 2000000000;

/* The global variable flexmap is a key-value list of addresses (keys)
 * and file paths (values).
 */
struct map *flexmap;
omp_nest_lock_t lock;

/* Flexmem initialization
 *
 * Initializes synchronizing lock variable and address/file key/value map.
 * This function may be called multiple times, be aware of that and keep
 * this as tiny/simple as possible and thread-safe.
 *
 */
static void
flexmem_init ()
{
#ifdef DEBUG
write(2,"INIT \n",6);
#endif
  if(READY < 1) omp_init_nest_lock (&lock);
  READY++;
  if(!flexmem_hook) flexmem_hook = __malloc_hook;
#ifdef DEBUG
write(2,"I1 \n",4);
#endif
  flexmem_default_free = (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
#ifdef DEBUG
write(2,"I2 \n",4);
#endif
}

/* Flexmem finalization
 * Remove any left over allocations, but we don't destroy the lock--should we?
 *
 */
static void
flexmem_finalize ()
{
  struct map *m, *tmp;
  READY = 0;
  omp_set_nest_lock (&lock);
  HASH_ITER(hh, flexmap, m, tmp)
  {
    munmap (m->addr, m->length);
#if defined(DEBUG) || defined(DEBUG2)
    fprintf(stderr,"Flexmem unmap address %p of size %lu\n", m->addr,
            (unsigned long int) m->length);
#endif
    unlink (m->path);
    HASH_DEL (flexmap, m);
    freemap (m);
  }
  omp_unset_nest_lock (&lock);
#if defined(DEBUG) || defined(DEBUG2)
  fprintf(stderr,"Flexmem finalized\n");
#endif
}


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

/* End of API functions ---------------------------------------------------- */



/* freemap is a utility function that deallocates the supplied map structure */
void
freemap (struct map *m)
{
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
  struct map *m, *y;
  void *x;
  int j;
  int fd;

  if(!flexmem_default_malloc)
    flexmem_default_malloc = (void *(*)(size_t)) dlsym (RTLD_NEXT, "malloc");
  if (size > flexmem_threshold && READY)
    {
      m = (struct map *) ((*flexmem_default_malloc) (sizeof (struct map)));
      m->path = (char *) ((*flexmem_default_malloc) (FLEXMEM_MAX_PATH_LEN));
      memset(m->path,0,FLEXMEM_MAX_PATH_LEN);
      omp_set_nest_lock (&lock);
      strncpy (m->path, flexmem_fname_template, FLEXMEM_MAX_PATH_LEN);
      m->length = size;
      fd = mkostemp (m->path, O_RDWR | O_CREAT);
      j = ftruncate (fd, m->length);
      if (j < 0)
        {
          omp_unset_nest_lock (&lock);
          close (fd);
          unlink (m->path);
          freemap (m);
          return NULL;
        }
      m->addr =
        mmap (NULL, m->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      x = m->addr;
      close (fd);
#if defined(DEBUG) || defined(DEBUG2)
      fprintf(stderr,"Flexmem malloc address %p, size %lu, file  %s\n", m->addr,
              (unsigned long int) m->length, m->path);
#endif
/* Check to make sure that this address is not already in the hash. If it is,
 * then something is terribly wrong and we must bail.
 */
      HASH_FIND_PTR (flexmap, m->addr, y);
      if(y)
      {
        munmap (m->addr, m->length);
        unlink (m->path);
        freemap (m);
        x = NULL;
      } else
      {
        HASH_ADD_PTR (flexmap, addr, m);
      }
#if defined(DEBUG) || defined(DEBUG2)
      fprintf(stderr,"hash count = %u\n", HASH_COUNT (flexmap));
#endif
      omp_unset_nest_lock (&lock);
    }
  else
    {
      x = (*flexmem_default_malloc) (size);
    }
#ifdef DEBUG
  fprintf(stderr,"malloc %p\n",x);
#endif
  return x;
}

void
free (void *ptr)
{
  struct map *m;
  if (!ptr)
    return;
  if (READY)
    {
#ifdef DEBUG
fprintf(stderr,"free %p \n",ptr);
#endif
      omp_set_nest_lock (&lock);
      HASH_FIND_PTR (flexmap, &ptr, m);
      if (m)
        {
          munmap (ptr, m->length);
#if defined(DEBUG) || defined(DEBUG2)
          fprintf(stderr,"Flexmem unmap address %p of size %lu\n", ptr,
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
#ifdef DEBUG
write(2,"F1 \n",4);
#endif
  (*flexmem_default_free) (ptr);
#ifdef DEBUG
write(2,"F2 \n",4);
#endif
}


void *
realloc (void *ptr, size_t size)
{
  struct map *m, *y;
  int j, fd;
  void *x;
#ifdef DEBUG
  fprintf(stderr,"realloc\n");
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
  if(!flexmem_default_realloc)
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
/* Check for existence of the address in the hash. It must not already exist,
 * (after all we just removed it and we hold the lock)--if it does something
 * is terribly wrong and we bail.
 */
          HASH_FIND_PTR (flexmap, &ptr, y);
          if(y)
          {
            munmap (m->addr, m->length);
            goto bail;
          }
          HASH_ADD_PTR (flexmap, addr, m);
          x = m->addr;
          close (fd);
#if defined(DEBUG) || defined(DEBUG2)
          fprintf(stderr,"Flexmem realloc address %p size %lu\n", ptr,
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
#if defined(DEBUG) || defined(DEBUG2)
      fprintf(stderr,"Flexmem calloc...handing off to flexmem malloc\n");
#endif
      return malloc (n);
    }
  if(!flexmem_hook) flexmem_init();
  x = flexmem_hook (n, NULL);
  memset (x, 0, n);
  return x;
}
