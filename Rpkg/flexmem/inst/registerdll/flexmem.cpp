#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#include <string>
using namespace std;

// Recursive mutexes are used to provide thread safety.
#include <boost/thread/recursive_mutex.hpp>
using namespace boost;

#include "flexmem.hpp"

MemoryMappedFileManager *mmfm;
string *flexmem_fname_template;

recursive_mutex& mem_mutex()
{
  static recursive_mutex m;
  return m;
}

extern "C" {

int flexmem_verbose = 0;
size_t flexmem_size_threshold = 4000000;
void (*flexmem_default_free) (void *);
void *(*flexmem_default_malloc) (size_t);
void *(*flexmem_default_realloc) (void *, size_t);
void *(*flexmem_default_calloc) (size_t, size_t);

void flexmem_set_threshold(size_t *j)
{
  if (*j > 0)
    flexmem_size_threshold = *j;
}

void flexmem_set_template(const char *name)
{
  *flexmem_fname_template = string(name);
}

void flexmem_set_verbose(int j)
{
  flexmem_verbose = j;
}

void flexmem_init()
{
  if (flexmem_verbose)
    printf("flexmem_init called\n");

  flexmem_default_free = (void (*)(void*))dlsym (RTLD_NEXT, "free");
  flexmem_default_malloc = (void *(*)(size_t)) dlsym (RTLD_NEXT, "malloc");
  flexmem_default_realloc =
    (void *(*)(void *, size_t)) dlsym (RTLD_NEXT, "realloc");
  flexmem_default_calloc =
    (void *(*)(size_t, size_t)) dlsym (RTLD_NEXT, "calloc");
  mmfm = new MemoryMappedFileManager();
  if (!flexmem_fname_template)
  {
    flexmem_fname_template = new string(getenv(
    flexmem_fname_template = new string("/tmp/FlexmemTempFileXXXXXX");
  
}

void* malloc(size_t size)
{
  lock_guard<recursive_mutex> _(mem_mutex());
  if (!flexmem_default_free)
    flexmem_init();

  if (size > flexmem_size_threshold)
  {
    string temp(*flexmem_fname_template);
    char *unused = mktemp(&(temp[0]));
    int p = getpagesize();
    void *pret;
    try {
      pret = mmfm->add(temp, size + p - (size % p));
    }
    catch(...) {
      return NULL;
    }
    if (flexmem_verbose) {
      printf("mmap malloc called with name %s for allocation of size %d\n",
        temp.c_str(), (int)size);
    }
    return pret;
  }
  if (flexmem_verbose)
    printf("malloc called for allocation of size %d\n", (int)size);
  return (*flexmem_default_malloc)(size);
}

void free(void *ptr)
{
  lock_guard<recursive_mutex> _(mem_mutex());
  if (!flexmem_default_free)
    flexmem_init();

  const MemoryMappedFile *pmmf = mmfm->find(ptr);
  if (NULL != pmmf)
  {
    if (flexmem_verbose)
      printf("mmap free called\n");
    mmfm->remove(ptr);
  }
  else
  {
    if (flexmem_verbose)
      printf("free called\n");
    (*flexmem_default_free)(ptr);
  }
}

void* realloc (void *ptr, size_t size)
{
  lock_guard<recursive_mutex> _(mem_mutex());
  if (NULL == ptr)
    return malloc(size);

  if (0 == size)
  {
    free(ptr);
    return NULL;
  }

  if (!flexmem_default_free)
    flexmem_init();

  const MemoryMappedFile *pmmf = mmfm->find(ptr);

  // If it wasn't memmory mapped and the remapped 
  // data isn't either call regular realloc.
  if ( (NULL != pmmf) && size < flexmem_size_threshold)
  {
    if (flexmem_verbose)
      printf("realloc called for memory of size %d.\n", (int)size);
    return (*flexmem_default_realloc)(ptr, size);
  }

  void *p;
  p = malloc(size);
  memcpy(p, ptr, size);
  if (flexmem_verbose)
    printf("realloc called for memory of size %d.\n", (int)size);
  free(ptr);
  return p;
}

void* flexmem_calloc(size_t count, size_t size)
{
  lock_guard<recursive_mutex> _(mem_mutex());
  if (flexmem_verbose)
    printf("calloc called... ");
  return malloc(count * size);
}

}
