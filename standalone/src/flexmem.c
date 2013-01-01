#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>

#include "uthash.h"
#include "flexmem.h"

static void flexmem_init (void) __attribute__ ((constructor));
static void flexmem_finalize (void) __attribute__ ((destructor));
static void *(*flexmem_hook) (size_t, const void *);
static void *(*flexmem_default_free) (void *);
static void *(*flexmem_default_malloc) (size_t);
static void *(*flexmem_default_valloc) (size_t);
static void *(*flexmem_default_realloc) (void *, size_t);

/* Additional library state */
void freemap (struct map *);
static int READY = 0;

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
  if(!flexmem_hook) flexmem_hook = __malloc_hook;
  flexmem_default_free = (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
  READY=1;
}

/* Flexmem finalization
 * Remove any left over allocations, but we don't destroy the lock--should we?
 *
 */
static void
flexmem_finalize ()
{
  struct map *m, *tmp;
  pid_t pid;
  READY = 0;
  omp_set_nest_lock (&lock);
  HASH_ITER(hh, flexmap, m, tmp)
  {
    munmap (m->addr, m->length);
#if defined(DEBUG) || defined(DEBUG2)
    fprintf(stderr,"Flexmem unmap address %p of size %lu\n", m->addr,
            (unsigned long int) m->length);
#endif
    pid = getpid();
    if(pid == m->pid)
    {
#if defined(DEBUG) || defined(DEBUG2)
      fprintf(stderr,"Flexmem ulink %s\n", m->path);
#endif
      unlink (m->path);
      HASH_DEL (flexmap, m);
      freemap (m);
    }
  }
  omp_unset_nest_lock (&lock);
#if defined(DEBUG) || defined(DEBUG2)
  fprintf(stderr,"Flexmem finalized\n");
#endif
}


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
      m->pid = getpid();
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
  pid_t pid;
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
/* Make sure a child process does not accidentally delete a mapping owned
 * by a parent.
 */
          pid = getpid();
          if(pid == m->pid)
          {
#if defined(DEBUG) || defined(DEBUG2)
          fprintf(stderr,"Flexmem ulink %p/%s\n", ptr, m->path);
#endif
            unlink (m->path);
            HASH_DEL (flexmap, m);
            freemap (m);
          }
          omp_unset_nest_lock (&lock);
          return;
        }
      omp_unset_nest_lock (&lock);
    }
  (*flexmem_default_free) (ptr);
}

/* valloc returns memory aligned to a page boundary.  Memory mapped flies are
 * aligned to page boundaries, so we simply return our modified malloc when
 * over the threshold. Otherwise, fall back to default valloc.
 */
void *
valloc (size_t size)
{
  if (READY && size > flexmem_threshold)
    {
#if defined(DEBUG) || defined(DEBUG2)
      fprintf(stderr,"Flexmem valloc...handing off to flexmem malloc\n");
#endif
      return malloc (size);
    }
  if(!flexmem_default_valloc)
    flexmem_default_valloc =
      (void *(*)(size_t)) dlsym (RTLD_NEXT, "valloc");
  return flexmem_default_valloc(size);
}

/* Realloc is complicated in the case of fork. We have to protect parents from
 * wayward children and also maintain expected realloc behavior. See comments
 * below...
 */
void *
realloc (void *ptr, size_t size)
{
  struct map *m, *y;
  int j, fd, child;
  void *x;
  pid_t pid;
  size_t copylen;
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
 * file mapping to the truncated file. But don't allow a child process
 * to screw with the parent's mapping.
 */
          munmap (ptr, m->length);
          pid = getpid();
          child = 0;
          if(pid == m->pid)
          {
            HASH_DEL (flexmap, m);
            m->length = size;
            fd = open (m->path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
          } else
          {
/* Uh oh. We're in a child process. We need to copy this mapping and create a
 * new map entry unique to the child.  Also  need to copy old data up to min
 * (size, m->length), this sucks.
 */
            y = m;
            child = 1;
            m = (struct map *) ((*flexmem_default_malloc) (sizeof (struct map)));
            m->path = (char *) ((*flexmem_default_malloc) (FLEXMEM_MAX_PATH_LEN));
            memset(m->path,0,FLEXMEM_MAX_PATH_LEN);
            strncpy (m->path, flexmem_fname_template, FLEXMEM_MAX_PATH_LEN);
            m->length = size;
            copylen = m->length;
            if(y->length < copylen) copylen = y->length;
            fd = mkostemp (m->path, O_RDWR | O_CREAT);
          }
          if (fd < 0)
            goto bail;
          j = ftruncate (fd, m->length);
          if (j < 0)
            goto bail;
          m->addr =
            mmap (NULL, m->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
/* Here is a rather unfortunate child copy... */
          if(child) memcpy(m->addr,y->addr,copylen);
          m->pid = getpid();
/* Check for existence of the address in the hash. It must not already exist,
 * (after all we just removed it and we hold the lock)--if it does something
 * is terribly wrong and we bail.
 */
          HASH_FIND_PTR (flexmap, m->addr, y);
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

#ifdef OSX
// XXX reallocf is a Mac OSX/BSD-specific function.
void *
reallocf (void *ptr, size_t size)
{
// XXX WRITE ME!
  return reallocf(ptr, size);
}
# else
// XXX memalign, posix_memalign are not in OSX/BSD, but are in Linux
// and POSIX. Define them here.
// XXX WRITE ME!
#endif


/* calloc is a special case. Unfortunately, dlsym ultimately calls calloc,
 * thus a direct interposition here results in an infinite loop...We created
 * a fake calloc that relies on malloc, and avoids looking it up by abusing the
 * malloc.h hooks library. This is much easier on BSD.
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
