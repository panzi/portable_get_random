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

Implementation
--------------

This library provides several different implementations that can be chosen from.
Per default it tries to automatically choose an implementation. This works for
Linux, Windows, OpenBSD, FreeBSD, macOS, iOS, Fuchsia, Haiku, and any other OS
that defines `__unix__` or `_POSIX_VERSION` and provides the special file
`/dev/random`.

You can control which implementation is to be used, though. Especially for Windows
that might be useful, if you want to use the new cryptography library (`bcrypt.h`),
which is only available on Windows 8 and newer.

To control which implementation to use call make e.g. like this:

```bash
make IMPL=BCryptGenRandom
```

If you've already successfully compiled with a different value of `IMPL` you need
to run `make clean` before compiling again in order for the change to have any
effect.

Possible values for `IMPL`:

| `IMPL`               | Default On                             | Description |
| -------------------- | -------------------------------------- | ----------- |
| `getentropy`         | OpenBSD, FreeBSD, DragonFly BSD, macOS | Use the `getentropy()` syscall (or C library function) provided by many Unix(-like) operating systems. |
| `getrandom`          | Linux, Solaris                         | Use the `getrandom()` Linux syscall with `GRND_RANDOM` flag. |
| `RtlGenRandom`       |                                        | Use `RtlGenRandom()` Win32 pseudo random number generator (very old function that might go away). |
| `CryptGenRandom`     | Windows                                | Use the deprecated Win32 cryptography API. |
| `BCryptGenRandom`    |                                        | Use the new Windows cryptography API only available on Windows 8 and newer. |
| `/dev/random`        | *nix, Haiku                            | Can be any `/dev/*` file. Read entropy from the given `/dev`-file. Default for any other OS defining the `__unix__` macro. |
| `SecRandomCopyBytes` | iOS                                    | Use the Apple Security framework. |
| `zx_cprng_draw`      | Fuchsia                                | Use the `zx_cprng_draw()` syscall provided by the Zircon kernel. |
| `dlsym`              |                                        | Use `dlsym()` to find the best option between `getrandom()`, `getentropy()`, and `/dev/random`. |
| `LoadLibrary`        |                                        | Use `LoadLibrary()` to find the best option between the deprecated Win32 cryptography API and `bcrypt.dll`. |
| `dynamic`            |                                        | Alias of `LoadLibrary` under Windows and of `dlsym` otherwise. |

If you drop these files into your own program and don't want to use the defaults
you need to add the compiler flag: `-DPORTABLE_GET_RANDOM_IMPL=PORTABLE_GET_RANDOM_IMPL_$IMPL`
That means if you want to use e.g. `RtlGenRandom` use the flag:
`-DPORTABLE_GET_RANDOM_IMPL=PORTABLE_GET_RANDOM_IMPL_RtlGenRandom` For `/dev/random`
it would be: `-DPORTABLE_GET_RANDOM_IMPL=PORTABLE_GET_RANDOM_IMPL_dev_random`
For this and `dlsym` you can define the extra flag `PORTABLE_GET_RANDOM_DEV_RANDOM`,
e.g.: `-DPORTABLE_GET_RANDOM_DEV_RANDOM='"/dev/urandom"'`

License
-------

[MIT License](#LICENSE.txt)
