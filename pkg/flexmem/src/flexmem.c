#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <R.h>
#define USE_RINTERNALS
#include <Rinternals.h>

/*
 * flexmem_threshold
 *
 * INPUT
 * J SEXP   Threshold value or R_NilValue to only report current threshold
 *
 * OUTPUT
 * SEXP     Threshold set point or R_NilValue on error
 *
 */

SEXP
flexmem_threshold (SEXP J)
{
  SEXP val;
  void *handle;
  size_t SIZE;
  void (*set_threshold)(size_t *);
  char *derror;

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }
  dlerror ();
  set_threshold = (void (*)(size_t*))dlsym(handle, "flexmem_set_threshold");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }

  PROTECT (val = allocVector(REALSXP, 1));
  SIZE = (size_t) *(REAL (J));
  (*set_threshold)(&SIZE);
  REAL(val)[0] = (double) SIZE;
  dlclose (handle);
  UNPROTECT (1);
  return (val);
}


/*
 * flexmem_template
 *
 * INPUT
 * S SEXP   A mktemp-style tempate string
 *
 * Always returns R_NilValue.
 */

SEXP
flexmem_template (SEXP S)
{
  void *handle;
  void (*set_template)(char *);
  char *derror;
  char *temp = (char *)CHAR (STRING_ELT (S, 0));

  handle = dlopen (NULL, RTLD_LAZY);
  if (!handle) {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }
  dlerror ();
  set_template = (void (*)(char *))dlsym(handle, "flexmem_set_template");
  if ((derror = dlerror ()) != NULL)  {
      error ("%s\n",dlerror ());
      return R_NilValue;
  }

  (*set_template)(temp);
  dlclose (handle);
  return (R_NilValue);
}

