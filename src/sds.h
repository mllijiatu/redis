/* SDSLib 2.0 -- A C dynamic strings library
 *
 * Copyright (c) 2006-2015, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2015, Oran Agra
 * Copyright (c) 2015, Redis Labs, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SDS_H
#define __SDS_H

#define SDS_MAX_PREALLOC (1024*1024)
extern const char *SDS_NOINIT;

#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>

// 简单 动态 字符串
typedef char *sds;

/* Note: sdshdr5 is never used, we just access the flags byte directly.
 * However is here to document the layout of type 5 SDS strings. */
struct __attribute__ ((__packed__)) sdshdr5 {
    unsigned char flags; /* 3 lsb of type, and 5 msb of string length */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr8 {
    uint8_t len; /* used */
    uint8_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 {
    uint16_t len; /* used */
    uint16_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
// __attribute__((__packed__)) 是一个 GNU C 扩展，用于告诉编译器不要在结构体中插入任何填充字节，以确保紧凑的内存布局。
// 记录长度，杜绝了缓冲区溢出。
// 惰性释放，减少重新分配的次数（使用记录将未使用的回收）
// 预分配，多给一些空间，避免多次扩容。
// 二进制安全，二进制安全（Binary Safety）是指一个系统或数据结构能够正确处理二进制数据，而不会在处理过程中受到特定字符或特殊字节的影响。
struct __attribute__ ((__packed__)) sdshdr32 {
    uint32_t len;       // 用于表示字符串的长度（已使用的长度）
    uint32_t alloc;     // 分配的内存大小（不包括头部和空终止符）
    unsigned char flags; // 3 个最低有效位表示类型，5 个未使用的位
    char buf[];         // 灵活数组成员，用于存储字符串数据
};
struct __attribute__ ((__packed__)) sdshdr64 {
    uint64_t len; /* used */
    uint64_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};

#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
// 这行代码是一个宏定义，用于创建一个掩码（mask）以提取 Simple Dynamic String（SDS）中存储类型信息的标志位的部分。具体来说，SDS_TYPE_MASK 被定义为数字 7，它在二进制中的表示是 00000111。
//
//在 SDS 的实现中，标志位的低 3 位用于存储 SDS 类型信息。通过使用 SDS_TYPE_MASK，可以方便地将标志位中的 SDS 类型信息提取出来。
#define SDS_TYPE_MASK 7
/**
 * 宏定义：SDS 字符串类型的标志位所占用的位数
 * 这个宏定义表示 SDS 类型信息所占用的位数，具体为 3 位。在 SDS 实现中，类型信息存储在标志位的低 3 位中，通过这个宏定义可以方便地引用 SDS 类型的位数。
 */
#define SDS_TYPE_BITS 3
// 这个宏定义用于声明一个 SDS 头部结构体的指针，并将指针初始化为指向 SDS 字符串起始位置之前的位置。T 是一个参数，用于指定 SDS 的类型（8、16、32、64）。
#define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T)));
// 这个宏定义用于返回一个指向 SDS 头部结构体的指针，同样根据 T 参数来选择不同的 SDS 类型。这个宏实际上是对宏（SDS_HDR_VAR）的简化，直接返回指针而不声明新的变量。
//
//这样的宏定义使得代码更具有通用性，能够方便地处理不同 SDS 类型的头部信息，同时也提高了代码的可读性和可维护性。
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
/**
 * 宏定义：计算 SDS_TYPE_5 类型的 SDS 字符串长度
 *
 * @param f SDS 字符串的标志位
 * @return SDS 字符串的长度
 */
#define SDS_TYPE_5_LEN(f) ((f) >> SDS_TYPE_BITS)

/**
 * 获取 SDS 字符串的长度
 *
 * @param s SDS 字符串
 * @return SDS 字符串的长度
 */
// inline 是一个关键字，用于告诉编译器尝试将函数内联展开，而不是按照常规的函数调用方式进行。这意味着编译器会尝试在调用该函数的地方直接插入函数的代码，而不是生成一个函数调用的指令。
//
//使用 inline 有助于减少函数调用的开销，因为它避免了函数调用的一些额外开销，如压栈、跳转等。函数内联展开可以提高程序的执行速度，特别是对于短小的函数或频繁调用的函数。
static inline size_t sdslen(const sds s) {
    // 从 SDS 字符串的头部获取存储的标志位
    unsigned char flags = s[-1];

    // 根据标志位的类型来决定如何计算字符串的长度
    switch(flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            // 对于 SDS_TYPE_5 类型，使用宏 SDS_TYPE_5_LEN 获取长度
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            // 对于 SDS_TYPE_8 类型，通过宏 SDS_HDR 获取头部信息，再获取长度
            return SDS_HDR(8, s)->len;
        case SDS_TYPE_16:
            // 对于 SDS_TYPE_16 类型，通过宏 SDS_HDR 获取头部信息，再获取长度
            return SDS_HDR(16, s)->len;
        case SDS_TYPE_32:
            // 对于 SDS_TYPE_32 类型，通过宏 SDS_HDR 获取头部信息，再获取长度
            return SDS_HDR(32, s)->len;
        case SDS_TYPE_64:
            // 对于 SDS_TYPE_64 类型，通过宏 SDS_HDR 获取头部信息，再获取长度
            return SDS_HDR(64, s)->len;
    }

    // 默认返回 0，表示无效长度
    return 0;
}


/**
 * 获取 SDS 字符串中剩余可用空间的大小
 *
 * @param s SDS 字符串
 * @return 剩余可用空间的大小
 */
static inline size_t sdsavail(const sds s) {
    // 从 SDS 字符串的头部获取存储的标志位
    unsigned char flags = s[-1];

    // 根据标志位的类型来决定如何计算剩余可用空间的大小
    switch(flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            // 对于 SDS_TYPE_5 类型，使用宏 SDS_TYPE_5_AVAIL 获取剩余可用空间的大小
            return SDS_TYPE_5_AVAIL(flags);
        case SDS_TYPE_8:
            // 对于 SDS_TYPE_8 类型，通过宏 SDS_HDR 获取头部信息，再获取剩余可用空间的大小
            return SDS_HDR(8, s)->alloc - SDS_HDR(8, s)->len;
        case SDS_TYPE_16:
            // 对于 SDS_TYPE_16 类型，通过宏 SDS_HDR 获取头部信息，再获取剩余可用空间的大小
            return SDS_HDR(16, s)->alloc - SDS_HDR(16, s)->len;
        case SDS_TYPE_32:
            // 对于 SDS_TYPE_32 类型，通过宏 SDS_HDR 获取头部信息，再获取剩余可用空间的大小
            return SDS_HDR(32, s)->alloc - SDS_HDR(32, s)->len;
        case SDS_TYPE_64:
            // 对于 SDS_TYPE_64 类型，通过宏 SDS_HDR 获取头部信息，再获取剩余可用空间的大小
            return SDS_HDR(64, s)->alloc - SDS_HDR(64, s)->len;
    }

    // 默认返回 0，表示无效大小
    return 0;
}


/**
 * 设置 SDS 字符串的长度
 *
 * @param s SDS 字符串
 * @param newlen 新的长度值
 */
static inline void sdssetlen(sds s, size_t newlen) {
    // 从 SDS 字符串的头部获取存储的标志位
    unsigned char flags = s[-1];

    // 根据标志位的类型来决定如何设置新的长度
    switch(flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5:
        {
            // 对于 SDS_TYPE_5 类型，通过指针运算获取标志位的地址，并更新长度信息
            unsigned char *fp = ((unsigned char*)s) - 1;
            *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
        }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len = newlen;
            break;
    }
}

/**
 * 增加 SDS 字符串的长度
 *
 * @param s SDS 字符串
 * @param inc 增加的长度值
 */
static inline void sdsinclen(sds s, size_t inc) {
    // 从 SDS 字符串的头部获取存储的标志位
    unsigned char flags = s[-1];

    // 根据标志位的类型来决定如何增加长度
    switch(flags & SDS_TYPE_MASK) {
        case SDS_TYPE_5:
        {
            // 对于 SDS_TYPE_5 类型，通过指针运算获取标志位的地址，并更新长度信息
            unsigned char *fp = ((unsigned char*)s) - 1;
            unsigned char newlen = SDS_TYPE_5_LEN(flags) + inc;
            *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
        }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len += inc;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len += inc;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len += inc;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len += inc;
            break;
    }
}

/* sdsalloc() = sdsavail() + sdslen() */
/**
 * 计算 SDS 字符串的总分配空间大小
 *
 * @param s SDS 字符串
 * @return SDS 字符串的总分配空间大小
 */
static inline size_t sdsalloc(const sds s) {
    // 从 SDS 字符串的头部获取存储的标志位
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->alloc;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->alloc;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->alloc;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->alloc;
    }
    return 0;
}

/**
 * 设置 SDS 字符串的总分配空间大小
 *
 * @param s SDS 字符串
 * @param newlen 新的总分配空间大小
 */
static inline void sdssetalloc(sds s, size_t newlen) {
    // 从 SDS 字符串的头部获取存储的标志位
    unsigned char flags = s[-1];

    // 根据标志位的类型来决定如何设置新的总分配空间大小
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            /* Nothing to do, this type has no total allocation info. */
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->alloc = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->alloc = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->alloc = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->alloc = newlen;
            break;
    }
}

sds sdsnewlen(const void *init, size_t initlen);
sds sdstrynewlen(const void *init, size_t initlen);
sds sdsnew(const char *init);
sds sdsempty(void);
sds sdsdup(const sds s);
void sdsfree(sds s);
sds sdsgrowzero(sds s, size_t len);
sds sdscatlen(sds s, const void *t, size_t len);
sds sdscat(sds s, const char *t);
sds sdscatsds(sds s, const sds t);
sds sdscpylen(sds s, const char *t, size_t len);
sds sdscpy(sds s, const char *t);

sds sdscatvprintf(sds s, const char *fmt, va_list ap);
#ifdef __GNUC__
sds sdscatprintf(sds s, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
#else
sds sdscatprintf(sds s, const char *fmt, ...);
#endif

sds sdscatfmt(sds s, char const *fmt, ...);
sds sdstrim(sds s, const char *cset);
void sdssubstr(sds s, size_t start, size_t len);
void sdsrange(sds s, ssize_t start, ssize_t end);
void sdsupdatelen(sds s);
void sdsclear(sds s);
int sdscmp(const sds s1, const sds s2);
sds *sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count);
void sdsfreesplitres(sds *tokens, int count);
void sdstolower(sds s);
void sdstoupper(sds s);
sds sdsfromlonglong(long long value);
sds sdscatrepr(sds s, const char *p, size_t len);
sds *sdssplitargs(const char *line, int *argc);
sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen);
sds sdsjoin(char **argv, int argc, char *sep);
sds sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen);
int sdsneedsrepr(const sds s);

/* Callback for sdstemplate. The function gets called by sdstemplate
 * every time a variable needs to be expanded. The variable name is
 * provided as variable, and the callback is expected to return a
 * substitution value. Returning a NULL indicates an error.
 */
typedef sds (*sdstemplate_callback_t)(const sds variable, void *arg);
sds sdstemplate(const char *template, sdstemplate_callback_t cb_func, void *cb_arg);

/* Low level functions exposed to the user API */
sds sdsMakeRoomFor(sds s, size_t addlen);
sds sdsMakeRoomForNonGreedy(sds s, size_t addlen);
void sdsIncrLen(sds s, ssize_t incr);
sds sdsRemoveFreeSpace(sds s, int would_regrow);
sds sdsResize(sds s, size_t size, int would_regrow);
size_t sdsAllocSize(sds s);
void *sdsAllocPtr(sds s);

/* Export the allocator used by SDS to the program using SDS.
 * Sometimes the program SDS is linked to, may use a different set of
 * allocators, but may want to allocate or free things that SDS will
 * respectively free or allocate. */
void *sds_malloc(size_t size);
void *sds_realloc(void *ptr, size_t size);
void sds_free(void *ptr);

#ifdef REDIS_TEST
int sdsTest(int argc, char *argv[], int flags);
#endif

#endif
