#include <dlfcn.h>

#include <R.h>
#define USE_RINTERNALS
#include <Rinternals.h>

/*
 * flexmem_threshold
 * INPUT * J SEXP   Threshold value or R_NilValue
 * OUTPUT * SEXP     Threshold set point or R_NilValue on error
 */
SEXP
Rflexmem_threshold (SEXP J)
{
  SEXP VAL;
  void *handle;
  size_t size;
  size_t (*set_threshold)(size_t );
  char *derror;

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }
  dlerror ();
  set_threshold = (size_t (*)(size_t ))dlsym(handle, "flexmem_set_threshold");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }
  dlclose (handle);

  PROTECT (VAL = allocVector(REALSXP, 1));
  size = (size_t) *(REAL (J));
  REAL(VAL)[0] = (double) (*set_threshold)(size);
  UNPROTECT (1);
  return (VAL);
}

/*
 * flexmem_set_pattern
 *
 * INPUT S SEXP   A mktemp-style tempate string
 * OUTPUT An SEXP integer, 0 for success otherwise error
 * int flexmem_set_pattern (char *pattern)
 *
 */
SEXP
Rflexmem_set_pattern (SEXP S)
{
  SEXP VAL;
  void *handle;
  int (*set_pattern)(char *);
  char *derror;
  char *temp = (char *)CHAR (STRING_ELT (S, 0));

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }
  dlerror ();
  set_pattern = (int (*)(char *))dlsym(handle, "flexmem_set_pattern");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }

  PROTECT (VAL = allocVector(INTSXP, 1));
  INTEGER(VAL)[0] = (int) (*set_pattern)(temp);
  UNPROTECT (1);
  dlclose (handle);
  return (VAL);
}

/*
 * flexmem_set_path
 *
 * INPUT S SEXP   A path to map files in
 * OUTPUT An SEXP integer, 0 for success otherwise error
 * int flexmem_set_path (char *path)
 *
 */
SEXP
Rflexmem_set_path (SEXP S)
{
  SEXP VAL;
  void *handle;
  int (*set_path)(char *);
  char *derror;
  char *temp = (char *)CHAR (STRING_ELT (S, 0));

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }
  dlerror ();
  set_path = (int (*)(char *))dlsym(handle, "flexmem_set_path");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }

  PROTECT (VAL = allocVector(INTSXP, 1));
  INTEGER(VAL)[0] = (int) (*set_path)(temp);
  UNPROTECT (1);
  dlclose (handle);
  return (VAL);
}


SEXP
Rflexmem_get_template ()
{
  SEXP VAL;
  void *handle;
  char * (*get_template)(void);
  char *derror;
  char *s;

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }
  dlerror ();
  get_template = (char *(*)(void))dlsym(handle, "flexmem_get_template");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }

  dlclose (handle);
  s = get_template();
  if(!s) return R_NilValue;
  PROTECT (VAL = mkString(s));
  UNPROTECT (1);
  free(s);
  return (VAL);
}

SEXP
Rflexmem_lookup (SEXP OBJECT)
{
  SEXP VAL;
  void *handle;
  char * (*flookup)(void *);
  char *derror, *s;

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }
  dlerror ();
  flookup = (char *(*)(void *))dlsym(handle, "flexmem_lookup");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }
  dlclose (handle);
  s = flookup((void *)OBJECT);
  if(!s) return R_NilValue;
  PROTECT (VAL = mkString(s));
  UNPROTECT (1);
  free(s);
  return (VAL);
}
