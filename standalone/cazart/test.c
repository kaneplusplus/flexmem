#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <dlfcn.h>

void testfunc(void);

int main (int argc, void **argv)
{
    int j;
    void *x;
    const char *y = "Cazart!";
    size_t SIZE = 1000000;

    void *handle;
    char *error;
    void (*testfunc)(void);
    handle = dlopen ("./testlib.so", RTLD_NOW);
    *(void **) (&testfunc) = dlsym(handle, "testfunc");
    if (!handle) {
        fprintf (stderr, "%s\n", dlerror());
        exit(1);
    }
    (*testfunc)();

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
