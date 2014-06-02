/*   ___    ___ _____ ______   _______   _____ ______      
 *  |\  \  /  /|\   _ \  _   \|\  ___ \ |\   _ \  _   \
 *  \ \  \/  / | \  \\\__\ \  \ \   __/|\ \  \\\__\ \  \
 *   \ \    / / \ \  \\|__| \  \ \  \_|/_\ \  \\|__| \  \
 *    /     \/   \ \  \    \ \  \ \  \_|\ \ \  \    \ \  \
 *   /  /\   \    \ \__\    \ \__\ \_______\ \__\    \ \__\
 *  /__/ /\ __\    \|__|     \|__|\|_______|\|__|     \|__|
 *  |__|/ \|__|                                            
 */                                                        
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

#define uthash_malloc(sz) uthash_malloc_(sz)
#define uthash_free(ptr, sz) uthash_free_(ptr)
#include "uthash.h"
#include "xmem.h"

extern void *__libc_malloc(size_t size);
static void xmem_init (void) __attribute__ ((constructor));
static void xmem_finalize (void) __attribute__ ((destructor));
static void *(*xmem_hook) (size_t);
static void *(*xmem_default_free) (void *);
static void *(*xmem_default_malloc) (size_t);
static void *(*xmem_default_valloc) (size_t);
static void *(*xmem_default_realloc) (void *, size_t);
static void *(*xmem_default_memcpy) (void *dest, const void *src, size_t n);

static void *uthash_malloc_ (size_t);
static void uthash_free_ (void *);
void freemap (struct map *);

/* READY has three states:
 * -1 at startup, prior to initialization of anything
 *  1 After initialization finished, ready to go.
 *  0 After finalization has been called, don't mmap anymore.
 */
static int READY = -1;

/* NOTES
 *
 * Xmem uses well-known methods to overload various memory allocation functions
 * such that allocations above a threshold use memory mapped files. The library
 * maintains a mapping of allocated addresses and corresponding backing files
 * and cleans up files as they are de-allocated.  The idea is to help programs
 * allocate large objects out of core while explicitly avoiding the system swap
 * space. The library provides a basic API that lets programs change the
 * mapping file path and threshold value.
 *
 * Be cautious when debugging about placement of printf, write, etc. These
 * things often end up re-entering one of our functions. The general rule here
 * is to keep things as minimal as possible.
 */

/* Xmem initialization
 *
 * Initializes synchronizing lock variable and address/file key/value map.
 * This function may be called multiple times, be aware of that and keep
 * this as tiny/simple as possible and thread-safe.
 *
 * Oddly, xmem_init is invoked lazily by calloc, which will be triggered
 * either directly by a program calloc call, or on first use of any of the
 * intercepted functions because all of them except for calloc call dlsym,
 * which itself calls calloc.
 *
 */
static void
xmem_init ()
{
#ifdef DEBUG
write(2,"INIT \n",6);
#endif
  if(READY < 0)
  {
    omp_init_nest_lock (&lock);
    READY=1;
  }
  if(!xmem_hook) xmem_hook = __libc_malloc;
  if(!xmem_default_free) xmem_default_free =
    (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
}

/* Xmem finalization
 * Remove any left over allocations, but we don't destroy the lock--XXX
 */
static void
xmem_finalize ()
{
  struct map *m, *tmp;
  pid_t pid;
  omp_set_nest_lock (&lock);
  READY = 0;
  HASH_ITER(hh, flexmap, m, tmp)
  {
    munmap (m->addr, m->length);
#if defined(DEBUG) || defined(DEBUG2)
    fprintf(stderr,"Xmem unmap address %p of size %lu\n", m->addr,
            (unsigned long int) m->length);
#endif
    pid = getpid();
    if(pid == m->pid)
    {
#if defined(DEBUG) || defined(DEBUG2)
      fprintf(stderr,"Xmem ulink %s\n", m->path);
#endif
      unlink (m->path);
      HASH_DEL (flexmap, m);
      freemap (m);
    }
  }
  omp_unset_nest_lock (&lock);
#if defined(DEBUG) || defined(DEBUG2)
  fprintf(stderr,"Xmem finalized\n");
#endif
}


/* freemap is a utility function that deallocates the supplied map structure */
void
freemap (struct map *m)
{
  if (m)
    {
      if (m->path)
        (*xmem_default_free) (m->path);
      (*xmem_default_free) (m);
    }
}

/* Make sure uthash uses the default malloc and free functions. */
void *
uthash_malloc_ (size_t size)
{
  if(!xmem_default_malloc)
    xmem_default_malloc = (void *(*)(size_t)) dlsym (RTLD_NEXT, "malloc");
  return (*xmem_default_malloc) (size);
}

void
uthash_free_ (void *ptr)
{
  if(!xmem_default_free)
    xmem_default_free = (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
  (*xmem_default_free) (ptr);
}

void *
malloc (size_t size)
{
  struct map *m, *y;
  void *x;
  int j;
  int fd;

  if(!xmem_default_malloc)
    xmem_default_malloc = (void *(*)(size_t)) dlsym (RTLD_NEXT, "malloc");
  if (size > xmem_threshold && READY>0)
    {
      m = (struct map *) ((*xmem_default_malloc) (sizeof (struct map)));
      m->path = (char *) ((*xmem_default_malloc) (XMEM_MAX_PATH_LEN));
      memset(m->path,0,XMEM_MAX_PATH_LEN);
      omp_set_nest_lock (&lock);
      strncpy (m->path, xmem_fname_template, XMEM_MAX_PATH_LEN);
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
      madvise(m->addr, m->length, xmem_advise);
      m->pid = getpid();
      x = m->addr;
      close (fd);
#if defined(DEBUG) || defined(DEBUG2)
      fprintf(stderr,"Xmem malloc address %p, size %lu, file  %s\n", m->addr,
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
      x = (*xmem_default_malloc) (size);
#ifdef DEBUG
      fprintf(stderr,"malloc %p\n",x);
#endif
    }
  return x;
}

void
free (void *ptr)
{
  struct map *m;
  pid_t pid;
  if (!ptr)
    return;
  if (READY>0)
    {
#ifdef DEBUG
fprintf(stderr,"free %p \n",ptr);
#endif
      omp_set_nest_lock (&lock);
      HASH_FIND_PTR (flexmap, &ptr, m);
      if (m)
        {
#if defined(DEBUG) || defined(DEBUG2)
          fprintf(stderr,"Xmem unmap address %p of size %lu\n", ptr,
                  (unsigned long int) m->length);
#endif
          munmap (ptr, m->length);
/* Make sure a child process does not accidentally delete a mapping owned
 * by a parent.
 */
          pid = getpid();
          if(pid == m->pid)
          {
#if defined(DEBUG) || defined(DEBUG2)
          fprintf(stderr,"Xmem ulink %p/%s\n", ptr, m->path);
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
  if(!xmem_default_free)
    xmem_default_free = (void *(*)(void *)) dlsym (RTLD_NEXT, "free");
  (*xmem_default_free) (ptr);
}

/* valloc returns memory aligned to a page boundary.  Memory mapped flies are
 * aligned to page boundaries, so we simply return our modified malloc when
 * over the threshold. Otherwise, fall back to default valloc.
 */
void *
valloc (size_t size)
{
  if (READY>0 && size > xmem_threshold)
    {
#if defined(DEBUG) || defined(DEBUG2)
      fprintf(stderr,"Xmem valloc...handing off to xmem malloc\n");
#endif
      return malloc (size);
    }
  if(!xmem_default_valloc)
    xmem_default_valloc =
      (void *(*)(size_t)) dlsym (RTLD_NEXT, "valloc");
  return xmem_default_valloc(size);
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
  if(!xmem_default_realloc)
    xmem_default_realloc =
      (void *(*)(void *, size_t)) dlsym (RTLD_NEXT, "realloc");
  if (READY>0)
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
            m = (struct map *) ((*xmem_default_malloc) (sizeof (struct map)));
            m->path = (char *) ((*xmem_default_malloc) (XMEM_MAX_PATH_LEN));
            memset(m->path,0,XMEM_MAX_PATH_LEN);
            strncpy (m->path, xmem_fname_template, XMEM_MAX_PATH_LEN);
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
/* Here is a rather unfortunate child copy... XXX ADAPT THIS TO USE A COW MAP */
          if(child) xmem_default_memcpy(m->addr,y->addr,copylen);
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
          fprintf(stderr,"Xmem realloc address %p size %lu\n", ptr,
                  (unsigned long int) m->length);
#endif
          omp_unset_nest_lock (&lock);
          return x;
        }
      omp_unset_nest_lock (&lock);
    }
  x = (*xmem_default_realloc) (ptr, size);
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


/* A xmem-aware memcpy.
 *
 * It turns out, at least on Linux, that memcpy on memory-mapped files is much
 * slower than simply copying the data with read and write--and much, much
 * slower than zero (user space) copy techniques using sendfile.
 *
 * We provide a custom memcpy that copies xmem-allocated regions. We use
 * read/write instead of sendfile because sometimes we only partially copy the
 * files.
 *
 * This routine can only copy entire files or portions of files defined by a
 * fixed-lengh offset (a common use case in R for example). Other kinds of
 * memcpy use the default memcpy.
 *
 * Many additional improvements are possible here. See the inline comments
 * below...
 */
void *
memcpy (void *dest, const void *src, size_t n)
{
  struct map *SRC, *DEST;
  void *dest_off;
  void *src_off;
  int src_fd, dest_fd;
  char buf[BUFSIZ];
  ssize_t s;
  if(!xmem_default_memcpy)
    xmem_default_memcpy =
      (void *(*)(void *, const void *, size_t)) dlsym (RTLD_NEXT, "memcpy");
  dest_off = (void *)( (char *)dest - xmem_offset);
  src_off  = (void *)( (char *)src  - xmem_offset);
  omp_set_nest_lock (&lock);
  HASH_FIND_PTR (flexmap, &src_off, SRC);
  HASH_FIND_PTR (flexmap, &dest_off, DEST);
  if (!SRC || !DEST)
  {
/* One or more of src, dest is not the start of a xmem allocation.
 * Default in this case to the usual memcpy.
 */
    omp_unset_nest_lock (&lock);
    return (*xmem_default_memcpy) (dest, src, n);
  }
  if(SRC->length != (n + xmem_offset) || DEST->length != (n+xmem_offset))
  {
/* Our efficient methods below require copy of a full region.
 * Default in this case to the usual memcpy.
 */
    omp_unset_nest_lock (&lock);
    return (*xmem_default_memcpy) (dest, src, n);
  }
#if defined(DEBUG) || defined(DEBUG2)
  fprintf(stderr,"CAZART! Xmem memcopy address %p src_addr %p of size %lu\n", SRC->addr, src,
            (unsigned long int) SRC->length);
#endif
/* XXX
what we really want here is to take the two file mappings and overlay them
explicitly at the file system or block device level. That is, put the SRC file
directly under the DEST file, then mark that it's a cow.

The problem with the OS cow is that changes to DEST will go into the
buffer cache instead of a file. If those changes are huge, they will
swap--mostly defeating our system.

So we need a magic overlay FS that can take two source files of the
same size and modify the 2nd file to be a COW mask above the 1st
file (all in place).

for now the best we can do is a reasonably efficient copy
*/
  src_fd = open(SRC->path,O_RDONLY);
  dest_fd = open(DEST->path,O_RDWR);
  omp_unset_nest_lock (&lock);
  lseek(src_fd, xmem_offset, SEEK_SET);
  lseek(dest_fd, xmem_offset, SEEK_SET);
  while ((s = read(src_fd, buf, BUFSIZ)) > 0) write(dest_fd, buf, s);
  return dest;
}




/* calloc is a special case. Unfortunately, dlsym ultimately calls calloc,
 * thus a direct interposition here results in an infinite loop...We created
 * a fake calloc that relies on malloc, and we avoid use of dlsym by using
 * the special glibc __libc_malloc. This works, but requires glibc!
 * We should not need to do this on BSD/OS X.
 */
void *
calloc (size_t count, size_t size)
{
  void *x;
  size_t n = count * size;
  if (READY>0 && n > xmem_threshold)
    {
#if defined(DEBUG) || defined(DEBUG2)
      fprintf(stderr,"Xmem calloc...handing off to xmem malloc\n");
#endif
      return malloc (n);
    }
  if(!xmem_hook) xmem_init();
  x = xmem_hook (n);//, NULL);
  memset (x, 0, n);
  return x;
}
