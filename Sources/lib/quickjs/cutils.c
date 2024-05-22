/*
 * C utilities
 *
 * Copyright (c) 2017 Fabrice Bellard
 * Copyright (c) 2018 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#if !defined(_MSC_VER)
#include <sys/time.h>
#endif

#include "cutils.h"

#undef NANOSEC
#define NANOSEC ((uint64_t) 1e9)

#pragma GCC visibility push(default)

void pstrcpy(char *buf, int buf_size, const char *str)
{
    int c;
    char *q = buf;

    if (buf_size <= 0)
        return;

    for(;;) {
        c = *str++;
        if (c == 0 || q >= buf + buf_size - 1)
            break;
        *q++ = c;
    }
    *q = '\0';
}

/* strcat and truncate. */
char *pstrcat(char *buf, int buf_size, const char *s)
{
    int len;
    len = strlen(buf);
    if (len < buf_size)
        pstrcpy(buf + len, buf_size - len, s);
    return buf;
}

int strstart(const char *str, const char *val, const char **ptr)
{
    const char *p, *q;
    p = str;
    q = val;
    while (*q != '\0') {
        if (*p != *q)
            return 0;
        p++;
        q++;
    }
    if (ptr)
        *ptr = p;
    return 1;
}

int has_suffix(const char *str, const char *suffix)
{
    size_t len = strlen(str);
    size_t slen = strlen(suffix);
    return (len >= slen && !memcmp(str + len - slen, suffix, slen));
}

/* Dynamic buffer package */

static void *dbuf_default_realloc(void *opaque, void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void dbuf_init2(DynBuf *s, void *opaque, DynBufReallocFunc *realloc_func)
{
    memset(s, 0, sizeof(*s));
    if (!realloc_func)
        realloc_func = dbuf_default_realloc;
    s->opaque = opaque;
    s->realloc_func = realloc_func;
}

void dbuf_init(DynBuf *s)
{
    dbuf_init2(s, NULL, NULL);
}

/* return < 0 if error */
int dbuf_realloc(DynBuf *s, size_t new_size)
{
    size_t size;
    uint8_t *new_buf;
    if (new_size > s->allocated_size) {
        if (s->error)
            return -1;
        size = s->allocated_size * 3 / 2;
        if (size > new_size)
            new_size = size;
        new_buf = s->realloc_func(s->opaque, s->buf, new_size);
        if (!new_buf) {
            s->error = TRUE;
            return -1;
        }
        s->buf = new_buf;
        s->allocated_size = new_size;
    }
    return 0;
}

int dbuf_write(DynBuf *s, size_t offset, const void *data, size_t len)
{
    size_t end;
    end = offset + len;
    if (dbuf_realloc(s, end))
        return -1;
    memcpy(s->buf + offset, data, len);
    if (end > s->size)
        s->size = end;
    return 0;
}

int dbuf_put(DynBuf *s, const void *data, size_t len)
{
    if (unlikely((s->size + len) > s->allocated_size)) {
        if (dbuf_realloc(s, s->size + len))
            return -1;
    }
    if (len > 0) {
        memcpy(s->buf + s->size, data, len);
        s->size += len;
    }
    return 0;
}

int dbuf_put_self(DynBuf *s, size_t offset, size_t len)
{
    if (unlikely((s->size + len) > s->allocated_size)) {
        if (dbuf_realloc(s, s->size + len))
            return -1;
    }
    memcpy(s->buf + s->size, s->buf + offset, len);
    s->size += len;
    return 0;
}

int dbuf_putc(DynBuf *s, uint8_t c)
{
    return dbuf_put(s, &c, 1);
}

int dbuf_putstr(DynBuf *s, const char *str)
{
    return dbuf_put(s, (const uint8_t *)str, strlen(str));
}

int __attribute__((format(printf, 2, 3))) dbuf_printf(DynBuf *s,
                                                      const char *fmt, ...)
{
    va_list ap;
    char buf[128];
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (len < sizeof(buf)) {
        /* fast case */
        return dbuf_put(s, (uint8_t *)buf, len);
    } else {
        if (dbuf_realloc(s, s->size + len + 1))
            return -1;
        va_start(ap, fmt);
        vsnprintf((char *)(s->buf + s->size), s->allocated_size - s->size,
                  fmt, ap);
        va_end(ap);
        s->size += len;
    }
    return 0;
}

void dbuf_free(DynBuf *s)
{
    /* we test s->buf as a fail safe to avoid crashing if dbuf_free()
       is called twice */
    if (s->buf) {
        s->realloc_func(s->opaque, s->buf, 0);
    }
    memset(s, 0, sizeof(*s));
}

/*--- Unicode / UTF-8 utility functions --*/

/* Note: at most 31 bits are encoded. At most UTF8_CHAR_LEN_MAX bytes
   are output. */
int unicode_to_utf8(uint8_t *buf, unsigned int c)
{
    uint8_t *q = buf;

    if (c < 0x80) {
        *q++ = c;
    } else {
        if (c < 0x800) {
            *q++ = (c >> 6) | 0xc0;
        } else {
            if (c < 0x10000) {
                *q++ = (c >> 12) | 0xe0;
            } else {
                if (c < 0x00200000) {
                    *q++ = (c >> 18) | 0xf0;
                } else {
                    if (c < 0x04000000) {
                        *q++ = (c >> 24) | 0xf8;
                    } else if (c < 0x80000000) {
                        *q++ = (c >> 30) | 0xfc;
                        *q++ = ((c >> 24) & 0x3f) | 0x80;
                    } else {
                        return 0;
                    }
                    *q++ = ((c >> 18) & 0x3f) | 0x80;
                }
                *q++ = ((c >> 12) & 0x3f) | 0x80;
            }
            *q++ = ((c >> 6) & 0x3f) | 0x80;
        }
        *q++ = (c & 0x3f) | 0x80;
    }
    return q - buf;
}

static const unsigned int utf8_min_code[5] = {
    0x80, 0x800, 0x10000, 0x00200000, 0x04000000,
};

static const unsigned char utf8_first_code_mask[5] = {
    0x1f, 0xf, 0x7, 0x3, 0x1,
};

/* return -1 if error. *pp is not updated in this case. max_len must
   be >= 1. The maximum length for a UTF8 byte sequence is 6 bytes. */
int unicode_from_utf8(const uint8_t *p, int max_len, const uint8_t **pp)
{
    int l, c, b, i;

    c = *p++;
    if (c < 0x80) {
        *pp = p;
        return c;
    }
    switch(c) {
    case 0xc0: case 0xc1: case 0xc2: case 0xc3:
    case 0xc4: case 0xc5: case 0xc6: case 0xc7:
    case 0xc8: case 0xc9: case 0xca: case 0xcb:
    case 0xcc: case 0xcd: case 0xce: case 0xcf:
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:
    case 0xd4: case 0xd5: case 0xd6: case 0xd7:
    case 0xd8: case 0xd9: case 0xda: case 0xdb:
    case 0xdc: case 0xdd: case 0xde: case 0xdf:
        l = 1;
        break;
    case 0xe0: case 0xe1: case 0xe2: case 0xe3:
    case 0xe4: case 0xe5: case 0xe6: case 0xe7:
    case 0xe8: case 0xe9: case 0xea: case 0xeb:
    case 0xec: case 0xed: case 0xee: case 0xef:
        l = 2;
        break;
    case 0xf0: case 0xf1: case 0xf2: case 0xf3:
    case 0xf4: case 0xf5: case 0xf6: case 0xf7:
        l = 3;
        break;
    case 0xf8: case 0xf9: case 0xfa: case 0xfb:
        l = 4;
        break;
    case 0xfc: case 0xfd:
        l = 5;
        break;
    default:
        return -1;
    }
    /* check that we have enough characters */
    if (l > (max_len - 1))
        return -1;
    c &= utf8_first_code_mask[l - 1];
    for(i = 0; i < l; i++) {
        b = *p++;
        if (b < 0x80 || b >= 0xc0)
            return -1;
        c = (c << 6) | (b & 0x3f);
    }
    if (c < utf8_min_code[l - 1])
        return -1;
    *pp = p;
    return c;
}

/*--- integer to string conversions --*/

/* All conversion functions:
   - require a destination array `buf` of sufficient length
   - write the string representation at the beginning of `buf`
   - null terminate the string
   - return the string length
 */

/* 2 <= base <= 36 */
char const digits36[36] = "0123456789abcdefghijklmnopqrstuvwxyz";

/* using u32toa_shift variant */

#define gen_digit(buf, c)  if (is_be()) \
            buf = (buf >> 8) | ((uint64_t)(c) << ((sizeof(buf) - 1) * 8)); \
        else \
            buf = (buf << 8) | (c)

size_t u7toa_shift(char dest[minimum_length(8)], uint32_t n)
{
    size_t len = 1;
    uint64_t buf = 0;
    while (n >= 10) {
        uint32_t quo = n % 10;
        n /= 10;
        gen_digit(buf, '0' + quo);
        len++;
    }
    gen_digit(buf, '0' + n);
    memcpy(dest, &buf, sizeof buf);
    return len;
}

size_t u07toa_shift(char dest[minimum_length(8)], uint32_t n, size_t len)
{
    size_t i;
    dest += len;
    dest[7] = '\0';
    for (i = 7; i-- > 1;) {
        uint32_t quo = n % 10;
        n /= 10;
        dest[i] = (char)('0' + quo);
    }
    dest[i] = (char)('0' + n);
    return len + 7;
}

size_t u32toa(char buf[minimum_length(11)], uint32_t n)
{
    if (n < 10) {
        buf[0] = (char)('0' + n);
        buf[1] = '\0';
        return 1;
    }
#define TEN_POW_7 10000000
    if (n >= TEN_POW_7) {
        uint32_t quo = n / TEN_POW_7;
        n %= TEN_POW_7;
        size_t len = u7toa_shift(buf, quo);
        return u07toa_shift(buf, n, len);
    }
    return u7toa_shift(buf, n);
}

size_t u64toa(char buf[minimum_length(21)], uint64_t n)
{
    if (likely(n < 0x100000000))
        return u32toa(buf, n);

    size_t len;
    if (n >= TEN_POW_7) {
        uint64_t n1 = n / TEN_POW_7;
        n %= TEN_POW_7;
        if (n1 >= TEN_POW_7) {
            uint32_t quo = n1 / TEN_POW_7;
            n1 %= TEN_POW_7;
            len = u7toa_shift(buf, quo);
            len = u07toa_shift(buf, n1, len);
        } else {
            len = u7toa_shift(buf, n1);
        }
        return u07toa_shift(buf, n, len);
    }
    return u7toa_shift(buf, n);
}

size_t i32toa(char buf[minimum_length(12)], int32_t n)
{
    if (likely(n >= 0))
        return u32toa(buf, n);

    buf[0] = '-';
    return 1 + u32toa(buf + 1, -(uint32_t)n);
}

size_t i64toa(char buf[minimum_length(22)], int64_t n)
{
    if (likely(n >= 0))
        return u64toa(buf, n);

    buf[0] = '-';
    return 1 + u64toa(buf + 1, -(uint64_t)n);
}

/* using u32toa_radix_length variant */

static uint8_t const radix_shift[64] = {
    0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

size_t u32toa_radix(char buf[minimum_length(33)], uint32_t n, unsigned base)
{
#ifdef USE_SPECIAL_RADIX_10
    if (likely(base == 10))
        return u32toa(buf, n);
#endif
    if (n < base) {
        buf[0] = digits36[n];
        buf[1] = '\0';
        return 1;
    }
    int shift = radix_shift[base & 63];
    if (shift) {
        uint32_t mask = (1 << shift) - 1;
        size_t len = (32 - clz32(n) + shift - 1) / shift;
        size_t last = n & mask;
        n /= base;
        char *end = buf + len;
        *end-- = '\0';
        *end-- = digits36[last];
        while (n >= base) {
            size_t quo = n & mask;
            n >>= shift;
            *end-- = digits36[quo];
        }
        *end = digits36[n];
        return len;
    } else {
        size_t len = 2;
        size_t last = n % base;
        n /= base;
        uint32_t nbase = base;
        while (n >= nbase) {
            nbase *= base;
            len++;
        }
        char *end = buf + len;
        *end-- = '\0';
        *end-- = digits36[last];
        while (n >= base) {
            size_t quo = n % base;
            n /= base;
            *end-- = digits36[quo];
        }
        *end = digits36[n];
        return len;
    }
}

size_t u64toa_radix(char buf[minimum_length(65)], uint64_t n, unsigned base)
{
#ifdef USE_SPECIAL_RADIX_10
    if (likely(base == 10))
        return u64toa(buf, n);
#endif
    int shift = radix_shift[base & 63];
    if (shift) {
        if (n < base) {
            buf[0] = digits36[n];
            buf[1] = '\0';
            return 1;
        }
        uint64_t mask = (1 << shift) - 1;
        size_t len = (64 - clz64(n) + shift - 1) / shift;
        size_t last = n & mask;
        n /= base;
        char *end = buf + len;
        *end-- = '\0';
        *end-- = digits36[last];
        while (n >= base) {
            size_t quo = n & mask;
            n >>= shift;
            *end-- = digits36[quo];
        }
        *end = digits36[n];
        return len;
    } else {
        if (likely(n < 0x100000000))
            return u32toa_radix(buf, n, base);
        size_t last = n % base;
        n /= base;
        uint64_t nbase = base;
        size_t len = 2;
        while (n >= nbase) {
            nbase *= base;
            len++;
        }
        char *end = buf + len;
        *end-- = '\0';
        *end-- = digits36[last];
        while (n >= base) {
            size_t quo = n % base;
            n /= base;
            *end-- = digits36[quo];
        }
        *end = digits36[n];
        return len;
    }
}

size_t i64toa_radix(char buf[minimum_length(66)], int64_t n, unsigned int base)
{
    if (likely(n >= 0))
        return u64toa_radix(buf, n, base);

    buf[0] = '-';
    return 1 + u64toa_radix(buf + 1, -(uint64_t)n, base);
}

/*---- sorting with opaque argument ----*/

typedef void (*exchange_f)(void *a, void *b, size_t size);
typedef int (*cmp_f)(const void *, const void *, void *opaque);

static void exchange_bytes(void *a, void *b, size_t size) {
    uint8_t *ap = (uint8_t *)a;
    uint8_t *bp = (uint8_t *)b;

    while (size-- != 0) {
        uint8_t t = *ap;
        *ap++ = *bp;
        *bp++ = t;
    }
}

static void exchange_one_byte(void *a, void *b, size_t size) {
    uint8_t *ap = (uint8_t *)a;
    uint8_t *bp = (uint8_t *)b;
    uint8_t t = *ap;
    *ap = *bp;
    *bp = t;
}

static void exchange_int16s(void *a, void *b, size_t size) {
    uint16_t *ap = (uint16_t *)a;
    uint16_t *bp = (uint16_t *)b;

    for (size /= sizeof(uint16_t); size-- != 0;) {
        uint16_t t = *ap;
        *ap++ = *bp;
        *bp++ = t;
    }
}

static void exchange_one_int16(void *a, void *b, size_t size) {
    uint16_t *ap = (uint16_t *)a;
    uint16_t *bp = (uint16_t *)b;
    uint16_t t = *ap;
    *ap = *bp;
    *bp = t;
}

static void exchange_int32s(void *a, void *b, size_t size) {
    uint32_t *ap = (uint32_t *)a;
    uint32_t *bp = (uint32_t *)b;

    for (size /= sizeof(uint32_t); size-- != 0;) {
        uint32_t t = *ap;
        *ap++ = *bp;
        *bp++ = t;
    }
}

static void exchange_one_int32(void *a, void *b, size_t size) {
    uint32_t *ap = (uint32_t *)a;
    uint32_t *bp = (uint32_t *)b;
    uint32_t t = *ap;
    *ap = *bp;
    *bp = t;
}

static void exchange_int64s(void *a, void *b, size_t size) {
    uint64_t *ap = (uint64_t *)a;
    uint64_t *bp = (uint64_t *)b;

    for (size /= sizeof(uint64_t); size-- != 0;) {
        uint64_t t = *ap;
        *ap++ = *bp;
        *bp++ = t;
    }
}

static void exchange_one_int64(void *a, void *b, size_t size) {
    uint64_t *ap = (uint64_t *)a;
    uint64_t *bp = (uint64_t *)b;
    uint64_t t = *ap;
    *ap = *bp;
    *bp = t;
}

static void exchange_int128s(void *a, void *b, size_t size) {
    uint64_t *ap = (uint64_t *)a;
    uint64_t *bp = (uint64_t *)b;

    for (size /= sizeof(uint64_t) * 2; size-- != 0; ap += 2, bp += 2) {
        uint64_t t = ap[0];
        uint64_t u = ap[1];
        ap[0] = bp[0];
        ap[1] = bp[1];
        bp[0] = t;
        bp[1] = u;
    }
}

static void exchange_one_int128(void *a, void *b, size_t size) {
    uint64_t *ap = (uint64_t *)a;
    uint64_t *bp = (uint64_t *)b;
    uint64_t t = ap[0];
    uint64_t u = ap[1];
    ap[0] = bp[0];
    ap[1] = bp[1];
    bp[0] = t;
    bp[1] = u;
}

static inline exchange_f exchange_func(const void *base, size_t size) {
    switch (((uintptr_t)base | (uintptr_t)size) & 15) {
    case 0:
        if (size == sizeof(uint64_t) * 2)
            return exchange_one_int128;
        else
            return exchange_int128s;
    case 8:
        if (size == sizeof(uint64_t))
            return exchange_one_int64;
        else
            return exchange_int64s;
    case 4:
    case 12:
        if (size == sizeof(uint32_t))
            return exchange_one_int32;
        else
            return exchange_int32s;
    case 2:
    case 6:
    case 10:
    case 14:
        if (size == sizeof(uint16_t))
            return exchange_one_int16;
        else
            return exchange_int16s;
    default:
        if (size == 1)
            return exchange_one_byte;
        else
            return exchange_bytes;
    }
}

static void heapsortx(void *base, size_t nmemb, size_t size, cmp_f cmp, void *opaque)
{
    uint8_t *basep = (uint8_t *)base;
    size_t i, n, c, r;
    exchange_f swap = exchange_func(base, size);

    if (nmemb > 1) {
        i = (nmemb / 2) * size;
        n = nmemb * size;

        while (i > 0) {
            i -= size;
            for (r = i; (c = r * 2 + size) < n; r = c) {
                if (c < n - size && cmp(basep + c, basep + c + size, opaque) <= 0)
                    c += size;
                if (cmp(basep + r, basep + c, opaque) > 0)
                    break;
                swap(basep + r, basep + c, size);
            }
        }
        for (i = n - size; i > 0; i -= size) {
            swap(basep, basep + i, size);

            for (r = 0; (c = r * 2 + size) < i; r = c) {
                if (c < i - size && cmp(basep + c, basep + c + size, opaque) <= 0)
                    c += size;
                if (cmp(basep + r, basep + c, opaque) > 0)
                    break;
                swap(basep + r, basep + c, size);
            }
        }
    }
}

static inline void *med3(void *a, void *b, void *c, cmp_f cmp, void *opaque)
{
    return cmp(a, b, opaque) < 0 ?
        (cmp(b, c, opaque) < 0 ? b : (cmp(a, c, opaque) < 0 ? c : a )) :
        (cmp(b, c, opaque) > 0 ? b : (cmp(a, c, opaque) < 0 ? a : c ));
}

/* pointer based version with local stack and insertion sort threshhold */
void rqsort(void *base, size_t nmemb, size_t size, cmp_f cmp, void *opaque)
{
    struct { uint8_t *base; size_t count; int depth; } stack[50], *sp = stack;
    uint8_t *ptr, *pi, *pj, *plt, *pgt, *top, *m;
    size_t m4, i, lt, gt, span, span2;
    int c, depth;
    exchange_f swap = exchange_func(base, size);
    exchange_f swap_block = exchange_func(base, size | 128);

    if (nmemb < 2 || size <= 0)
        return;

    sp->base = (uint8_t *)base;
    sp->count = nmemb;
    sp->depth = 0;
    sp++;

    while (sp > stack) {
        sp--;
        ptr = sp->base;
        nmemb = sp->count;
        depth = sp->depth;

        while (nmemb > 6) {
            if (++depth > 50) {
                /* depth check to ensure worst case logarithmic time */
                heapsortx(ptr, nmemb, size, cmp, opaque);
                nmemb = 0;
                break;
            }
            /* select median of 3 from 1/4, 1/2, 3/4 positions */
            /* should use median of 5 or 9? */
            m4 = (nmemb >> 2) * size;
            m = med3(ptr + m4, ptr + 2 * m4, ptr + 3 * m4, cmp, opaque);
            swap(ptr, m, size);  /* move the pivot to the start or the array */
            i = lt = 1;
            pi = plt = ptr + size;
            gt = nmemb;
            pj = pgt = top = ptr + nmemb * size;
            for (;;) {
                while (pi < pj && (c = cmp(ptr, pi, opaque)) >= 0) {
                    if (c == 0) {
                        swap(plt, pi, size);
                        lt++;
                        plt += size;
                    }
                    i++;
                    pi += size;
                }
                while (pi < (pj -= size) && (c = cmp(ptr, pj, opaque)) <= 0) {
                    if (c == 0) {
                        gt--;
                        pgt -= size;
                        swap(pgt, pj, size);
                    }
                }
                if (pi >= pj)
                    break;
                swap(pi, pj, size);
                i++;
                pi += size;
            }
            /* array has 4 parts:
             * from 0 to lt excluded: elements identical to pivot
             * from lt to pi excluded: elements smaller than pivot
             * from pi to gt excluded: elements greater than pivot
             * from gt to n excluded: elements identical to pivot
             */
            /* move elements identical to pivot in the middle of the array: */
            /* swap values in ranges [0..lt[ and [i-lt..i[
               swapping the smallest span between lt and i-lt is sufficient
             */
            span = plt - ptr;
            span2 = pi - plt;
            lt = i - lt;
            if (span > span2)
                span = span2;
            swap_block(ptr, pi - span, span);
            /* swap values in ranges [gt..top[ and [i..top-(top-gt)[
               swapping the smallest span between top-gt and gt-i is sufficient
             */
            span = top - pgt;
            span2 = pgt - pi;
            pgt = top - span2;
            gt = nmemb - (gt - i);
            if (span > span2)
                span = span2;
            swap_block(pi, top - span, span);

            /* now array has 3 parts:
             * from 0 to lt excluded: elements smaller than pivot
             * from lt to gt excluded: elements identical to pivot
             * from gt to n excluded: elements greater than pivot
             */
            /* stack the larger segment and keep processing the smaller one
               to minimize stack use for pathological distributions */
            if (lt > nmemb - gt) {
                sp->base = ptr;
                sp->count = lt;
                sp->depth = depth;
                sp++;
                ptr = pgt;
                nmemb -= gt;
            } else {
                sp->base = pgt;
                sp->count = nmemb - gt;
                sp->depth = depth;
                sp++;
                nmemb = lt;
            }
        }
        /* Use insertion sort for small fragments */
        for (pi = ptr + size, top = ptr + nmemb * size; pi < top; pi += size) {
            for (pj = pi; pj > ptr && cmp(pj - size, pj, opaque) > 0; pj -= size)
                swap(pj, pj - size, size);
        }
    }
}

/*---- Portable time functions ----*/

#if defined(_MSC_VER)
 // From: https://stackoverflow.com/a/26085827
static int gettimeofday_msvc(struct timeval *tp, struct timezone *tzp)
{
  static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

  SYSTEMTIME  system_time;
  FILETIME    file_time;
  uint64_t    time;

  GetSystemTime(&system_time);
  SystemTimeToFileTime(&system_time, &file_time);
  time = ((uint64_t)file_time.dwLowDateTime);
  time += ((uint64_t)file_time.dwHighDateTime) << 32;

  tp->tv_sec = (long)((time - EPOCH) / 10000000L);
  tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

  return 0;
}

uint64_t js__hrtime_ns(void) {
    LARGE_INTEGER counter, frequency;
    double scaled_freq;
    double result;

    if (!QueryPerformanceFrequency(&frequency))
        abort();
    assert(frequency.QuadPart != 0);

    if (!QueryPerformanceCounter(&counter))
        abort();
    assert(counter.QuadPart != 0);

  /* Because we have no guarantee about the order of magnitude of the
   * performance counter interval, integer math could cause this computation
   * to overflow. Therefore we resort to floating point math.
   */
  scaled_freq = (double) frequency.QuadPart / NANOSEC;
  result = (double) counter.QuadPart / scaled_freq;
  return (uint64_t) result;
}
#else
uint64_t js__hrtime_ns(void) {
  struct timespec t;

  if (clock_gettime(CLOCK_MONOTONIC, &t))
    abort();

  return t.tv_sec * NANOSEC + t.tv_nsec;
}
#endif

int64_t js__gettimeofday_us(void) {
    struct timeval tv;
#if defined(_MSC_VER)
    gettimeofday_msvc(&tv, NULL);
#else
    gettimeofday(&tv, NULL);
#endif
    return ((int64_t)tv.tv_sec * 1000000) + tv.tv_usec;
}

/*--- Cross-platform threading APIs. ----*/

#if !defined(EMSCRIPTEN) && !defined(__wasi__)

#if defined(_WIN32)
typedef void (*js__once_cb)(void);

typedef struct {
    js__once_cb callback;
} js__once_data_t;

static BOOL WINAPI js__once_inner(INIT_ONCE *once, void *param, void **context) {
    js__once_data_t *data = param;

    data->callback();

    return TRUE;
}

void js_once(js_once_t *guard, js__once_cb callback) {
    js__once_data_t data = { .callback = callback };
    InitOnceExecuteOnce(guard, js__once_inner, (void*) &data, NULL);
}

void js_mutex_init(js_mutex_t *mutex) {
    InitializeCriticalSection(mutex);
}

void js_mutex_destroy(js_mutex_t *mutex) {
    DeleteCriticalSection(mutex);
}

void js_mutex_lock(js_mutex_t *mutex) {
    EnterCriticalSection(mutex);
}

void js_mutex_unlock(js_mutex_t *mutex) {
    LeaveCriticalSection(mutex);
}

void js_cond_init(js_cond_t *cond) {
    InitializeConditionVariable(cond);
}

void js_cond_destroy(js_cond_t *cond) {
  /* nothing to do */
  (void) cond;
}

void js_cond_signal(js_cond_t *cond) {
    WakeConditionVariable(cond);
}

void js_cond_broadcast(js_cond_t *cond) {
    WakeAllConditionVariable(cond);
}

void js_cond_wait(js_cond_t *cond, js_mutex_t *mutex) {
    if (!SleepConditionVariableCS(cond, mutex, INFINITE))
        abort();
}

int js_cond_timedwait(js_cond_t *cond, js_mutex_t *mutex, uint64_t timeout) {
    if (SleepConditionVariableCS(cond, mutex, (DWORD)(timeout / 1e6)))
        return 0;
    if (GetLastError() != ERROR_TIMEOUT)
        abort();
    return -1;
}

#else /* !defined(_WIN32) */

void js_once(js_once_t *guard, void (*callback)(void)) {
    if (pthread_once(guard, callback))
        abort();
}

void js_mutex_init(js_mutex_t *mutex) {
    if (pthread_mutex_init(mutex, NULL))
        abort();
}

void js_mutex_destroy(js_mutex_t *mutex) {
    if (pthread_mutex_destroy(mutex))
        abort();
}

void js_mutex_lock(js_mutex_t *mutex) {
    if (pthread_mutex_lock(mutex))
        abort();
}

void js_mutex_unlock(js_mutex_t *mutex) {
    if (pthread_mutex_unlock(mutex))
        abort();
}

void js_cond_init(js_cond_t *cond) {
#if defined(__APPLE__) && defined(__MACH__)
    if (pthread_cond_init(cond, NULL))
        abort();
#else
    pthread_condattr_t attr;

    if (pthread_condattr_init(&attr))
        abort();

    if (pthread_condattr_setclock(&attr, CLOCK_MONOTONIC))
        abort();

    if (pthread_cond_init(cond, &attr))
        abort();

    if (pthread_condattr_destroy(&attr))
        abort();
#endif
}

void js_cond_destroy(js_cond_t *cond) {
#if defined(__APPLE__) && defined(__MACH__)
    /* It has been reported that destroying condition variables that have been
     * signalled but not waited on can sometimes result in application crashes.
     * See https://codereview.chromium.org/1323293005.
     */
    pthread_mutex_t mutex;
    struct timespec ts;
    int err;

    if (pthread_mutex_init(&mutex, NULL))
        abort();

    if (pthread_mutex_lock(&mutex))
        abort();

    ts.tv_sec = 0;
    ts.tv_nsec = 1;

    err = pthread_cond_timedwait_relative_np(cond, &mutex, &ts);
    if (err != 0 && err != ETIMEDOUT)
        abort();

    if (pthread_mutex_unlock(&mutex))
        abort();

    if (pthread_mutex_destroy(&mutex))
        abort();
#endif /* defined(__APPLE__) && defined(__MACH__) */

    if (pthread_cond_destroy(cond))
        abort();
}

void js_cond_signal(js_cond_t *cond) {
    if (pthread_cond_signal(cond))
        abort();
}

void js_cond_broadcast(js_cond_t *cond) {
    if (pthread_cond_broadcast(cond))
        abort();
}

void js_cond_wait(js_cond_t *cond, js_mutex_t *mutex) {
#if defined(__APPLE__) && defined(__MACH__)
    int r;

    errno = 0;
    r = pthread_cond_wait(cond, mutex);

    /* Workaround for a bug in OS X at least up to 13.6
     * See https://github.com/libuv/libuv/issues/4165
     */
    if (r == EINVAL && errno == EBUSY)
        return;
    if (r)
        abort();
#else
    if (pthread_cond_wait(cond, mutex))
        abort();
#endif
}

int js_cond_timedwait(js_cond_t *cond, js_mutex_t *mutex, uint64_t timeout) {
    int r;
    struct timespec ts;

#if !defined(__APPLE__)
    timeout += js__hrtime_ns();
#endif

    ts.tv_sec = timeout / NANOSEC;
    ts.tv_nsec = timeout % NANOSEC;
#if defined(__APPLE__) && defined(__MACH__)
    r = pthread_cond_timedwait_relative_np(cond, mutex, &ts);
#else
    r = pthread_cond_timedwait(cond, mutex, &ts);
#endif

    if (r == 0)
        return 0;

    if (r == ETIMEDOUT)
        return -1;

    abort();

    /* Pacify some compilers. */
    return -1;
}

#endif

#endif /* !defined(EMSCRIPTEN) && !defined(__wasi__) */

#pragma GCC visibility pop