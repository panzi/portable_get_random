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

So far only tested on Linux, WINE, and macOS.

* [Setup and Compilation](#setup-and-compilation)
  * [Compile static library](#compile-static-library)
    * [Compile statically linked examples](#compile-statically-linked-examples)
  * [Compile shared library](#compile-shared-library)
    * [Compile dynamically linked examples](#compile-dynamically-linked-examples)
  * [Compile Release](#compile-release)
  * [Cross Compilation](#cross-compilation)
* [Implementation](#implementation)
* [License](#license)

Setup and Compilation
---------------------

This library has no dependencies so no setup is needed. You can even just
drop the header and source file into your own project and are done using
the defaults. If you don't want to use defaults see:
[Implementation](#implementation).

### Compile static library

```bash
make
```

(or `make static`)

This will generate these files:

* `build/$target/$release/lib/libportable-get-random.a`
* `build/$target/$release/include/portable_get_random.h`

Where `$release` is either `debug` or `release` (see also
[Compile Release](#compile-release)).

#### Compile statically linked examples

```bash
make examples
```

This will generate these files:

* `build/$target/$release/examples/getrandom`

Note that under macOS there are no truly statically linked binaries that link
the standard C library. Therefore for macOS this will be a dynamically linked
binary, but the object files of `libportable-get-random.a` will be compiled
into to binary like any other object files directly belonging to it.

You can compile examples that way for other platforms by passing `PSEUDO_STATIC=ON`:

```bash
make PSEUDO_STATIC=ON examples
```

### Compile shared library

```bash
make shared
```

This will generate these files:

* `build/$target/$release/lib/libportable-get-random.so`
* `build/$target/$release/include/portable_get_random.h`

#### Compile dynamically linked examples

```bash
make examples_shared
```

This will generate these files:

* `build/$target/$release/examples-shared/getrandom`

To run the dynamically linked example program you need to put the
compiled shared libraries into your `$LD_LIBRARY_PATH`, e.g.:

```bash
export LD_LIBRARY_PATH=$PWD/build/linux64/debug/lib:$LD_LIBRARY_PATH
```

### Compile Release

Per default a debug configuration will be compiled. To compile in release mode
you just need to run:

```bash
make RELEASE=ON
```

### Cross Compilation

For cross compilation set the flag `TARGET` to values like `win32`, `win64`,
`linux32`, or `linux64`. Per default (for non-cross compilation) `TARGET` is:
`$(uname|tr '[:upper:]' '[:lower:]')$(getconf LONG_BIT)`

Example:

```bash
make TARGET=win32
```

Cross compiling to Windows on Linux requires `i686-w64-mingw32-gcc` or
`x86_64-w64-mingw32-gcc`. Other than that case cross compilation just means
to compile a 32 bit binary on a 64 bit system, but targeting the same operating
system and base processor architecture.

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
| `/dev/random`        | POSIX, Haiku                           | Can be any `/dev/*` file. Read entropy from the given `/dev`-file. Default for any other OS defining the `__unix__` macro. |
| `SecRandomCopyBytes` | iOS                                    | Use the Apple Security framework. |
| `zx_cprng_draw`      | Fuchsia                                | Use the `zx_cprng_draw()` syscall provided by the Zircon kernel. |
| `dlsym`              |                                        | Use `dlsym()` to find the best option between `SecRandomCopyBytes()`, `getrandom()`, `getentropy()`, and `/dev/random`. |
| `LoadLibrary`        |                                        | Use `LoadLibrary()` to find the best option between `CryptGenRandom()`, and `BCryptGenRandom()`. |
| `dynamic`            |                                        | Alias of `LoadLibrary` under Windows, alias of `dlsym` anywhere else except for Fuchsia, where it just defaults to `zx_cprng_draw`. |

If you drop these files into your own program and don't want to use the defaults
you need to add the compiler flag: `-DPORTABLE_GET_RANDOM_IMPL=PORTABLE_GET_RANDOM_IMPL_$IMPL`
That means if you want to use e.g. `RtlGenRandom` use the flag:
`-DPORTABLE_GET_RANDOM_IMPL=PORTABLE_GET_RANDOM_IMPL_RtlGenRandom` For `/dev/random`
it would be: `-DPORTABLE_GET_RANDOM_IMPL=PORTABLE_GET_RANDOM_IMPL_dev_random`
For this and `dlsym` you can define the extra flag `PORTABLE_GET_RANDOM_FILE`,
e.g.: `-DPORTABLE_GET_RANDOM_FILE='"/dev/urandom"'`

License
-------

[MIT License](#LICENSE.txt)
