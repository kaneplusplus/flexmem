#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>

int main (int argc, void **argv)
{
    int j;
    void *x;
    const char *y = "Cazart!";
    size_t SIZE = 1000000;

    void *handle;
    void (*set_threshold)(size_t *);
    void (*set_template)(char *);
    char *error;
    handle = dlopen (NULL, RTLD_LAZY);
    if (!handle) {
        fprintf (stderr, "%s\n", dlerror());
        exit(1);
    }
    dlerror(); 
    *(void **) (&set_threshold) = dlsym(handle, "flexmem_set_threshold");
    if ((error = dlerror()) != NULL)  {
        fprintf (stderr, "%s\n", error);
        exit(1);
    }
    *(void **) (&set_template) = dlsym(handle, "flexmem_set_template");
    if ((error = dlerror()) != NULL)  {
        fprintf (stderr, "%s\n", error);
        exit(1);
    }

    (*set_threshold)(&SIZE);
    printf("> API returns SIZE=%lu\n",SIZE);
    dlclose (handle);

    printf("> malloc below threshold\n");
    x = malloc(SIZE - 1);
    memcpy (x, (const void *) y, strlen (y));
    printf ("> (press a key to continue)\n");
    getc(stdin);
    free (x);

    printf("> malloc above threshold\n");
    x = malloc(SIZE + 1);
    memcpy (x, (const void *) y, strlen (y));
    printf ("> (press a key to continue)\n");
    getc(stdin);
    free (x);


    printf("> calloc below threshold\n");
    x = calloc(SIZE - 1,1);
    memcpy (x, (const void *) y, strlen (y));
    printf ("> (press a key to continue)\n");
    getc(stdin);
    free (x);

    printf("> calloc above threshold\n");
    x = calloc(SIZE + 1,1);
    memcpy (x, (const void *) y, strlen (y));
    printf ("> (press a key to continue)\n");
    getc(stdin);
    free (x);


    printf("> malloc + realloc below threshold\n");
    x = malloc(SIZE - 1);
    x = realloc(x, SIZE + 1);
    memcpy (x, (const void *) y, strlen (y));
    printf ("> (press a key to continue)\n");
    getc(stdin);
    free (x);

    printf("> malloc + realloc above threshold\n");
    x = malloc(SIZE + 1);
    x = realloc(x, SIZE + 10);
    memcpy (x, (const void *) y, strlen (y));
    printf ("> (press a key to continue)\n");
    getc(stdin);
    free (x);


    return 0;
}
