// Copyright 2021 Mathias Panzenböck
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "portable_get_random.h"

#define PORTABLE_GET_RANDOM_IMPL_getentropy         1
#define PORTABLE_GET_RANDOM_IMPL_getrandom          2
#define PORTABLE_GET_RANDOM_IMPL_dev_random         3
#define PORTABLE_GET_RANDOM_IMPL_RtlGenRandom       4
#define PORTABLE_GET_RANDOM_IMPL_CryptGenRandom     5
#define PORTABLE_GET_RANDOM_IMPL_BCryptGenRandom    6
#define PORTABLE_GET_RANDOM_IMPL_SecRandomCopyBytes 7
#define PORTABLE_GET_RANDOM_IMPL_zx_cprng_draw      8
#define PORTABLE_GET_RANDOM_IMPL_dlsym              9
#define PORTABLE_GET_RANDOM_IMPL_LoadLibrary       10

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)
    #define PORTABLE_GET_RANDOM_IMPL_dynamic PORTABLE_GET_RANDOM_IMPL_LoadLibrary
#else
    #define PORTABLE_GET_RANDOM_IMPL_dynamic PORTABLE_GET_RANDOM_IMPL_dlsym
#endif

#if !defined(PORTABLE_GET_RANDOM_DEV_RANDOM)
    #define PORTABLE_GET_RANDOM_DEV_RANDOM "/dev/random"
#endif

#if !defined(PORTABLE_GET_RANDOM_USE)
    #if defined(__linux__)
        // we compile with -std=c99 but here we do want getentropy()
        #define _DEFAULT_SOURCE 1

        #include <sys/syscall.h>

        #if defined(SYS_getrandom)
            #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_getrandom
        #else
            #include <features.h>

            #if defined(__GLIBC__) && (__GLIBC__ > 2 || __GLIBC__ == 2 && __GLIBC_MINOR__ >= 25)
                #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_getentropy
            #else
                #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_dev_random
            #endif
        #endif
    #elif defined(__CYGWIN__) || (defined(__sun) && defined(__SVR4))
        #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_getrandom
    #elif defined(__APPLE__) && defined(__MACH__)
        #include <TargetConditionals.h>

        #if TARGET_OS_IPHONE
            #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_SecRandomCopyBytes
        #else
            #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_getentropy
        #endif
    #elif defined(__FreeBSD__) || defined(__DragonFly__)
        #include <sys/param.h>
        #if (defined(__FreeBSD_version) && __FreeBSD_version >= 1200000) || \
            (defined(__DragonFly_version) && __DragonFly_version >= 500700)
            #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_getrandom
        #else
            #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_dev_random
        #endif
    #elif defined(__OpenBSD__)
        #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_getentropy
    #elif defined(_WIN32) || defined(_WIN64)
        #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_CryptGenRandom
    #elif defined(__Fuchsia__)
        #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_zx_cprng_draw
    #elif defined(__unix__) || defined(_POSIX_VERSION) || defined(__HAIKU__)
        #define PORTABLE_GET_RANDOM_USE PORTABLE_GET_RANDOM_IMPL_dev_random
    #else
        #error "System not supported by portable_get_random()."
    #endif
#endif

#if PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_getrandom

    #include <errno.h>
    #include <sys/random.h>

int portable_get_random(unsigned char *buffer, size_t size) {
    while (size > 0) {
        const ssize_t count = getrandom(buffer, size, GRND_RANDOM);
        if (count < 0) {
            const int errnum = errno;
            if (errnum == EINTR || errnum == EAGAIN) {
                continue;
            }
            return errnum;
        }
        buffer += count;
        size   -= count;
    }
    return 0;
}

#elif PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_getentropy

    // we compile with -std=c99 but here we do want getentropy()
    #if !defined(_DEFAULT_SOURCE)
        #define _DEFAULT_SOURCE 1
    #endif

    #include <errno.h>
    #include <unistd.h>

    #define MAX_ENTROPY_SIZE 256

int portable_get_random(unsigned char *buffer, size_t size) {
    while (size > 0) {
        const size_t count = size < MAX_ENTROPY_SIZE ? size : MAX_ENTROPY_SIZE;

        if (getentropy(buffer, count) != 0) {
            const int errnum = errno;
            if (errnum == EINTR || errnum == EAGAIN) {
                continue;
            }
            return errnum;
        }
        
        buffer += count;
        size   -= count;
    }
    return 0;
}

#elif PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_dlsym

enum PortableGetRandom_Impl {
    PortableGetRandom_Uninitialized,
    PortableGetRandom_GetRandom,
    PortableGetRandom_GetEntropy,
    PortableGetRandom_DevRandom,
};

    #include <dlfcn.h>
    #include <errno.h>
    #include <stdio.h>
    #include <unistd.h>

    #define MAX_ENTROPY_SIZE 256
    #define GRND_RANDOM 2

int portable_get_random(unsigned char *buffer, size_t size) {
    static enum PortableGetRandom_Impl impl = PortableGetRandom_Uninitialized;
    static ssize_t (*getrandom)(void *, size_t, unsigned int) = NULL;
    static int (*getentropy)(void *, size_t) = NULL;

dispatch:
    switch (impl) {
        case PortableGetRandom_Uninitialized:
            *(void**) &getrandom = dlsym(NULL, "getrandom");
            if (getrandom) {
                impl = PortableGetRandom_GetRandom;
                goto dispatch;
            }

            *(void**) &getentropy = dlsym(NULL, "getentropy");
            if (getentropy) {
                impl = PortableGetRandom_GetEntropy;
                goto dispatch;
            }

            impl = PortableGetRandom_DevRandom;
            goto dispatch;
            break;

        case PortableGetRandom_GetRandom:
            while (size > 0) {
                const ssize_t count = getrandom(buffer, size, GRND_RANDOM);
                if (count < 0) {
                    const int errnum = errno;
                    if (errnum == EINTR || errnum == EAGAIN) {
                        continue;
                    }
                    return errnum;
                }
                buffer += count;
                size   -= count;
            }
            return 0;

        case PortableGetRandom_GetEntropy:
            while (size > 0) {
                const size_t count = size < MAX_ENTROPY_SIZE ? size : MAX_ENTROPY_SIZE;

                if (getentropy(buffer, count) != 0) {
                    const int errnum = errno;
                    if (errnum == EINTR || errnum == EAGAIN) {
                        continue;
                    }
                    return errnum;
                }
                
                buffer += count;
                size   -= count;
            }
            return 0;

        case PortableGetRandom_DevRandom:
        {
            FILE *stream = fopen(PORTABLE_GET_RANDOM_DEV_RANDOM, "rb");

            if (stream == NULL) {
                return errno;
            }

            const size_t count = fread(buffer, 1, (size_t)size, stream);
            if (count < (size_t)size) {
                const int errnum = errno;
                fclose(stream);
                return errnum;
            }

            fclose(stream);

            return 0;
        }
        default:
            return ENOSYS;
    }
}

#elif (PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_RtlGenRandom) || \
      (PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_CryptGenRandom) || \
      (PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_BCryptGenRandom) || \
      (PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_LoadLibrary)

    #include <windows.h>

    #if PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_RtlGenRandom

        #include <ntsecapi.h>
        #include <errno.h>

int portable_get_random(unsigned char *buffer, size_t size) {
    if (!RtlGenRandom(buffer, size)) {
        return EINVAL;
    }
    return 0;
}

    #elif PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_BCryptGenRandom

        // Has nothing to do with bcrypt. It is the new ("better") Windows cryptography API.
        #include <bcrypt.h>
        #include <ntstatus.h>
        #include <errno.h>

// CryptGenRandom() is deprecated. Windows 8 and later provide BCryptGenRandom()
// which should be used instead.
int portable_get_random(unsigned char *buffer, size_t size) {
    NTSTATUS status = BCryptGenRandom(NULL, buffer, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    if (status != STATUS_SUCCESS) {
        switch (status) {
            case STATUS_INVALID_HANDLE:
                return EBADF;

            case STATUS_INVALID_PARAMETER:
            default:
                return EINVAL;
        }
    }

    return 0;
}

    #else

        #include <wincrypt.h>
        #include <errno.h>

        #if PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_LoadLibrary

            #include <ntstatus.h>

enum PortableGetRandom_Impl {
    PortableGetRandom_Uninitialized,
    PortableGetRandom_Crypt,
    PortableGetRandom_BCrypt,
};

static int portable_get_random_crypt(unsigned char *buffer, size_t size);

int portable_get_random(unsigned char *buffer, size_t size) {
    static enum PortableGetRandom_Impl impl = PortableGetRandom_Uninitialized;
    static NTSTATUS (WINAPI *BCryptGenRandom)(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG) = NULL;

dispatch:
    switch (impl) {
        case PortableGetRandom_Uninitialized:
        {
            HMODULE bcrypt = LoadLibraryA("bcrypt.dll");
            if (bcrypt == NULL) {
                impl = PortableGetRandom_Crypt;
                goto dispatch;
            }

            BCryptGenRandom = (NTSTATUS (WINAPI*)(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG))GetProcAddress(bcrypt, "BCryptGenRandom");
            if (BCryptGenRandom == NULL) {
                impl = PortableGetRandom_Crypt;
                goto dispatch;
            }

            impl = PortableGetRandom_BCrypt;
            goto dispatch;
            break;
        }

        case PortableGetRandom_Crypt:
            return portable_get_random_crypt(buffer, size);

        case PortableGetRandom_BCrypt:
        {
            NTSTATUS status = BCryptGenRandom(NULL, buffer, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

            if (status != STATUS_SUCCESS) {
                switch (status) {
                    case STATUS_INVALID_HANDLE:
                        return EBADF;

                    case STATUS_INVALID_PARAMETER:
                    default:
                        return EINVAL;
                }
            }
            return 0;
        }

        default:
            return ENOSYS;
    }
}

int portable_get_random_crypt(unsigned char *buffer, size_t size) {
        #else
int portable_get_random(unsigned char *buffer, size_t size) {
        #endif
    HCRYPTPROV hCryptProv = (HCRYPTPROV)-1;
    DWORD error = ERROR_SUCCESS;

    if (!CryptAcquireContext(
            &hCryptProv,
            NULL,
            NULL,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT)) {
        error = GetLastError();
        goto cleanup;
    }

    if (!CryptGenRandom(hCryptProv, size, buffer)) {
        error = GetLastError();
        goto cleanup;
    }

cleanup:
    if (hCryptProv != (HCRYPTPROV)-1) {
        CryptReleaseContext(hCryptProv, 0);
    }

    switch (error) {
        case ERROR_SUCCESS:
            return 0;

        case ERROR_BUSY:
            return EBUSY;

        case ERROR_FILE_NOT_FOUND:
            return ENOENT;

        case ERROR_NOT_ENOUGH_MEMORY:
            return ENOMEM;

        case NTE_BAD_FLAGS:
        case NTE_BAD_KEY_STATE:
        case NTE_BAD_KEYSET:
        case NTE_BAD_KEYSET_PARAM:
        case NTE_BAD_PROV_TYPE:
        case NTE_BAD_SIGNATURE:
            return EINVAL;

        case NTE_EXISTS:
            return EEXIST;

        case NTE_KEYSET_ENTRY_BAD:
            return EINVAL;

        case NTE_KEYSET_NOT_DEF:
            return ENOENT;

        case NTE_NO_MEMORY:
            return ENOMEM;

        case NTE_PROV_DLL_NOT_FOUND:
            return ENOENT;

        case NTE_PROV_TYPE_ENTRY_BAD:
        case NTE_PROV_TYPE_NO_MATCH:
        case NTE_PROV_TYPE_NOT_DEF:
        case NTE_PROVIDER_DLL_FAIL:
        case NTE_SIGNATURE_FILE_BAD:
            return EINVAL;

        case ERROR_INVALID_HANDLE:
        case NTE_BAD_UID:
            return EBADF;

        case ERROR_INVALID_PARAMETER:
        case NTE_FAIL:
        default:
            return EINVAL;
    }
}

    #endif

#elif PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_SecRandomCopyBytes

    #include <stdint.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <inttypes.h>

    #include <Security/SecBase.h>
    #include <Security/SecRandom.h>
    #include <CoreFoundation/CoreFoundation.h>

int portable_get_random(unsigned char *buffer, size_t size) {
    int status = SecRandomCopyBytes(kSecRandomDefault, size, buffer);

    switch (status) {
        case errSecSuccess:
            return 0;

        case errSecUnimplemented:
            return ENOSYS;

        case errSecDiskFull:
            return EDQUOT;

        case errSecIO:
            return EIO;

        case errSecAllocate:
            return ENOMEM;

        case errSecWrPerm:
            return EACCES;

        case errSecParam:
        default:
            return EINVAL;
    }
}

#elif PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_zx_cprng_draw

    #include <zircon/syscalls.h>

int portable_get_random(unsigned char *buffer, size_t size) {
    zx_cprng_draw(buffer, size);
    return 0;
}

#elif PORTABLE_GET_RANDOM_USE == PORTABLE_GET_RANDOM_IMPL_dev_random

    #include <errno.h>
    #include <stdio.h>

int portable_get_random(unsigned char *buffer, size_t size) {
    FILE *stream = fopen(PORTABLE_GET_RANDOM_DEV_RANDOM, "rb");

    if (stream == NULL) {
        return errno;
    }

    const size_t count = fread(buffer, 1, (size_t)size, stream);
    if (count < (size_t)size) {
        const int errnum = errno;
        fclose(stream);
        return errnum;
    }

    fclose(stream);

    return 0;
}

#else

    #error "PORTABLE_GET_RANDOM_USE has an invalid value."

#endif
