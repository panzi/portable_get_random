portable_get_random
===================

This library provides a single function:

```C
int portable_get_random(unsigned char *buffer, size_t size);
```

This function can be used to get cryptographically random bytes on various
operating systems.

On success the return value is 0, otherwise it is an `errno.h` error value.
Under non-POSIX operating systems operating system errors are converted to a
POSIX error. This of course looses details (e.g. many Windows or macOS errors
are mapped to the same POSIX error). If there is no good mapping `EINVAL` is
used.

So far only tested on Linux and WINE.

[MIT License](#LICENSE.txt)
