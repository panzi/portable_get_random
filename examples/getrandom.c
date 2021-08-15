#include <portable_get_random.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#if defined(_WIN32) || defined(_WIN64)
    #if defined(__MINGW32__) || defined(__MINGW64__)
        #include <inttypes.h>
    #elif defined(_WIN64)
        #define PRIuPTR "I64u"
    #elif defined(_WIN32)
        #define PRIuPTR "I32u"
    #endif
#else
    #include <inttypes.h>
#endif

void usage(int argc, char *argv[]) {
    const char *progname = argc > 0 ? argv[0] : "getrandom";
    printf("usage: %s <size>\n", progname);
}

int main(int argc, char *argv[]) {
    unsigned char *buffer = NULL;
    int status = 0;

    if (argc != 2) {
        usage(argc, argv);
        goto error;
    }

    char *endptr = NULL;
    const unsigned long long ullsize = strtoull(argv[1], &endptr, 10);
    if (!*argv[1] || *endptr || ullsize > SIZE_MAX) {
        fprintf(stderr, "*** error: illegal size: %s\n", argv[1]);
        goto error;
    }

    const size_t size = ullsize;
    buffer = malloc(size);
    if (buffer == NULL) {
        fprintf(stderr, "*** error: malloc(%" PRIuPTR "): %s\n", size, strerror(errno));
        goto error;
    }

    int errnum = portable_get_random(buffer, size);
    errnum = portable_get_random(buffer, size);
    if (errnum != 0) {
        fprintf(stderr, "*** error: portable_get_random(buffer, %" PRIuPTR "): %s\n", size, strerror(errno));
        goto error;
    }

    fwrite(buffer, 1, size, stdout);
    fflush(stdout);

    goto cleanup;

error:
    status = 1;

cleanup:
    free(buffer);

    return status;
}
