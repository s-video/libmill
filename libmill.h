/*

  Copyright (c) 2015 Martin Sustrik

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom
  the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

#ifndef LIBMILL_H_INCLUDED
#define LIBMILL_H_INCLUDED

#include <errno.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

/******************************************************************************/
/*  ABI versioning support                                                    */
/******************************************************************************/

/*  Don't change this unless you know exactly what you're doing and have      */
/*  read and understand the following documents:                              */
/*  www.gnu.org/software/libtool/manual/html_node/Libtool-versioning.html     */
/*  www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html  */

/*  The current interface version. */
#define MILL_VERSION_CURRENT 19

/*  The latest revision of the current interface. */
#define MILL_VERSION_REVISION 1

/*  How many past interface versions are still supported. */
#define MILL_VERSION_AGE 1

/******************************************************************************/
/*  Symbol visibility                                                         */
/******************************************************************************/

#if !defined __GNUC__ && !defined __clang__
#error "Unsupported compiler!"
#endif

#if defined MILL_NO_EXPORTS
#   define MILL_EXPORT
#else
#   if defined _WIN32
#      if defined MILL_EXPORTS
#          define MILL_EXPORT __declspec(dllexport)
#      else
#          define MILL_EXPORT __declspec(dllimport)
#      endif
#   else
#      if defined __SUNPRO_C
#          define MILL_EXPORT __global
#      elif (defined __GNUC__ && __GNUC__ >= 4) || \
             defined __INTEL_COMPILER || defined __clang__
#          define MILL_EXPORT __attribute__ ((visibility("default")))
#      else
#          define MILL_EXPORT
#      endif
#   endif
#endif

/******************************************************************************/
/*  Helpers                                                                   */
/******************************************************************************/

#define mill_string2_(x) #x
#define mill_string1_(x) mill_string2_(x)
#define MILL_HERE_ (__FILE__ ":" mill_string1_(__LINE__))

#define mill_concat_(x,y) x##y

MILL_EXPORT int64_t mill_now_(
    void);
MILL_EXPORT pid_t mill_mfork_(
    void);

#if defined MILL_USE_PREFIX
#define mill_now mill_now_
#define mill_mfork mill_mfork_
#else
#define now mill_now_
#define mfork mill_mfork_
#endif

/******************************************************************************/
/*  Coroutines                                                                */
/******************************************************************************/

#define MILL_FDW_IN_ 1
#define MILL_FDW_OUT_ 2
#define MILL_FDW_ERR_ 4

MILL_EXPORT extern volatile int mill_unoptimisable1_;
MILL_EXPORT extern volatile void *mill_unoptimisable2_;

#if defined __x86_64__
typedef uint64_t *mill_ctx;
#else
typedef sigjmp_buf *mill_ctx;
#endif

MILL_EXPORT mill_ctx mill_getctx_(
    void);
MILL_EXPORT __attribute__((noinline)) void *mill_prologue_(
    const char *created);
MILL_EXPORT __attribute__((noinline)) void mill_epilogue_(
    void);

MILL_EXPORT void mill_goprepare_(
    int count,
    size_t stack_size,
    size_t val_size);
MILL_EXPORT void mill_yield_(
    const char *current);
MILL_EXPORT void mill_msleep_(
    int64_t deadline,
    const char *current);
MILL_EXPORT void mill_fdclean_(
    int fd);
MILL_EXPORT void *mill_cls_(
    void);
MILL_EXPORT void mill_setcls_(
    void *val);


#if defined(__x86_64__)
#if defined(__AVX__)
#define MILL_CLOBBER \
        , "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7",\
        "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14", "ymm15"
#else
#define MILL_CLOBBER
#endif
#define mill_setjmp_(ctx) ({\
    int ret;\
    asm("lea     LJMPRET%=(%%rip), %%rcx\n\t"\
        "xor     %%rax, %%rax\n\t"\
        "mov     %%rbx, (%%rdx)\n\t"\
        "mov     %%rbp, 8(%%rdx)\n\t"\
        "mov     %%r12, 16(%%rdx)\n\t"\
        "mov     %%rsp, 24(%%rdx)\n\t"\
        "mov     %%r13, 32(%%rdx)\n\t"\
        "mov     %%r14, 40(%%rdx)\n\t"\
        "mov     %%r15, 48(%%rdx)\n\t"\
        "mov     %%rcx, 56(%%rdx)\n\t"\
        "mov     %%rdi, 64(%%rdx)\n\t"\
        "mov     %%rsi, 72(%%rdx)\n\t"\
        "LJMPRET%=:\n\t"\
        : "=a" (ret)\
        : "d" (ctx)\
        : "memory", "rcx", "r8", "r9", "r10", "r11",\
          "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",\
          "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"\
          MILL_CLOBBER\
          );\
    ret;\
})
#define mill_longjmp_(ctx) \
    asm("movq   (%%rax), %%rbx\n\t"\
	    "movq   8(%%rax), %%rbp\n\t"\
	    "movq   16(%%rax), %%r12\n\t"\
	    "movq   24(%%rax), %%rdx\n\t"\
	    "movq   32(%%rax), %%r13\n\t"\
	    "movq   40(%%rax), %%r14\n\t"\
	    "mov    %%rdx, %%rsp\n\t"\
	    "movq   48(%%rax), %%r15\n\t"\
	    "movq   56(%%rax), %%rdx\n\t"\
	    "movq   64(%%rax), %%rdi\n\t"\
	    "movq   72(%%rax), %%rsi\n\t"\
	    "jmp    *%%rdx\n\t"\
        : : "a" (ctx) : "rdx" \
    )
#else
#define mill_setjmp_(ctx) \
    sigsetjmp(*ctx, 0)
#define mill_longjmp_(ctx) \
    siglongjmp(*ctx, 1)
#endif

#define mill_go_(fn) \
    do {\
        void *mill_sp;\
        mill_ctx ctx = mill_getctx_();\
        if(!mill_setjmp_(ctx)) {\
            mill_sp = mill_prologue_(MILL_HERE_);\
            int mill_anchor[mill_unoptimisable1_];\
            mill_unoptimisable2_ = &mill_anchor;\
            char mill_filler[(char*)&mill_anchor - (char*)(mill_sp)];\
            mill_unoptimisable2_ = &mill_filler;\
            fn;\
            mill_epilogue_();\
        }\
    } while(0)

#if defined MILL_USE_PREFIX
#define MILL_FDW_IN MILL_FDW_IN_
#define MILL_FDW_OUT MILL_FDW_OUT_
#define MILL_FDW_ERR MILL_FDW_ERR_
#define mill_coroutine __attribute__((noinline))
#define mill_go(fn) mill_go_(fn)
#define mill_goprepare mill_goprepare_
#define mill_yield() mill_yield_(MILL_HERE_)
#define mill_msleep(dd) mill_msleep_((dd), MILL_HERE_)
#define mill_fdclean mill_fdclean_
#define mill_cls mill_cls_
#define mill_setcls mill_setcls_
#else
#define FDW_IN MILL_FDW_IN_
#define FDW_OUT MILL_FDW_OUT_
#define FDW_ERR MILL_FDW_ERR_
#define coroutine __attribute__((noinline))
#define go(fn) mill_go_(fn)
#define goprepare mill_goprepare_
#define yield() mill_yield_(MILL_HERE_)
#define msleep(deadline) mill_msleep_((deadline), MILL_HERE_)
#define fdclean mill_fdclean_
#define cls mill_cls_
#define setcls mill_setcls_
#endif

/******************************************************************************/
/*  Channels                                                                  */
/******************************************************************************/

struct mill_chan_;

typedef struct{void *f1; void *f2; void *f3; void *f4;
    void *f5; void *f6; int f7; int f8; int f9;} mill_clause_;
#define MILL_CLAUSELEN_ (sizeof(mill_clause_))

MILL_EXPORT struct mill_chan_ *mill_chmake_(
    size_t sz,
    size_t bufsz,
    const char *created);
MILL_EXPORT struct mill_chan_ *mill_chdup_(
    struct mill_chan_ *ch,
    const char *created);
MILL_EXPORT void mill_chclose_(
    struct mill_chan_ *ch,
    const char *current);
MILL_EXPORT void mill_chs_(
    struct mill_chan_ *ch,
    void *val,
    size_t sz,
    const char *current);
MILL_EXPORT void *mill_chr_(
    struct mill_chan_ *ch,
    size_t sz,
    const char *current);
MILL_EXPORT void mill_chdone_(
    struct mill_chan_ *ch,
    void *val,
    size_t sz,
    const char *current);
MILL_EXPORT void mill_choose_init_(
    const char *current);
MILL_EXPORT void mill_choose_in_(
    void *clause,
    struct mill_chan_ *ch,
    size_t sz,
    int idx);
MILL_EXPORT void mill_choose_out_(
    void *clause,
    struct mill_chan_ *ch,
    void *val,
    size_t sz,
    int idx);
MILL_EXPORT void mill_choose_deadline_(
    int64_t deadline);
MILL_EXPORT void mill_choose_otherwise_(
    void);
MILL_EXPORT int mill_choose_wait_(
    void);
MILL_EXPORT void *mill_choose_val_(
    size_t sz);

#define mill_chs__(channel, type, value) \
    do {\
        type mill_val = (value);\
        mill_chs_((channel), &mill_val, sizeof(type), MILL_HERE_);\
    } while(0)

#define mill_chr__(channel, type) \
    (*(type*)mill_chr_((channel), sizeof(type), MILL_HERE_))

#define mill_chdone__(channel, type, value) \
    do {\
        type mill_val = (value);\
        mill_chdone_((channel), &mill_val, sizeof(type), MILL_HERE_);\
    } while(0)

#define mill_choose_init__ \
    {\
        mill_choose_init_(MILL_HERE_);\
        int mill_idx = -2;\
        while(1) {\
            if(mill_idx != -2) {\
                if(0)

#define mill_choose_in__(chan, type, name, idx) \
                    break;\
                }\
                goto mill_concat_(mill_label, idx);\
            }\
            char mill_concat_(mill_clause, idx)[MILL_CLAUSELEN_];\
            mill_choose_in_(\
                &mill_concat_(mill_clause, idx)[0],\
                (chan),\
                sizeof(type),\
                idx);\
            if(0) {\
                type name;\
                mill_concat_(mill_label, idx):\
                if(mill_idx == idx) {\
                    name = *(type*)mill_choose_val_(sizeof(type));\
                    goto mill_concat_(mill_dummylabel, idx);\
                    mill_concat_(mill_dummylabel, idx)

#define mill_choose_out__(chan, type, val, idx) \
                    break;\
                }\
                goto mill_concat_(mill_label, idx);\
            }\
            char mill_concat_(mill_clause, idx)[MILL_CLAUSELEN_];\
            type mill_concat_(mill_val, idx) = (val);\
            mill_choose_out_(\
                &mill_concat_(mill_clause, idx)[0],\
                (chan),\
                &mill_concat_(mill_val, idx),\
                sizeof(type),\
                idx);\
            if(0) {\
                mill_concat_(mill_label, idx):\
                if(mill_idx == idx) {\
                    goto mill_concat_(mill_dummylabel, idx);\
                    mill_concat_(mill_dummylabel, idx)

#define mill_choose_deadline__(ddline, idx) \
                    break;\
                }\
                goto mill_concat_(mill_label, idx);\
            }\
            mill_choose_deadline_(ddline);\
            if(0) {\
                mill_concat_(mill_label, idx):\
                if(mill_idx == -1) {\
                    goto mill_concat_(mill_dummylabel, idx);\
                    mill_concat_(mill_dummylabel, idx)

#define mill_choose_otherwise__(idx) \
                    break;\
                }\
                goto mill_concat_(mill_label, idx);\
            }\
            mill_choose_otherwise_();\
            if(0) {\
                mill_concat_(mill_label, idx):\
                if(mill_idx == -1) {\
                    goto mill_concat_(mill_dummylabel, idx);\
                    mill_concat_(mill_dummylabel, idx)

#define mill_choose_end__ \
                    break;\
                }\
            }\
            mill_idx = mill_choose_wait_();\
        }

#if defined MILL_USE_PREFIX
typedef struct mill_chan_ *mill_chan;
#define mill_chmake(tp, sz) mill_chmake_(sizeof(tp), sz, MILL_HERE_)
#define mill_chdup(ch) mill_chdup_((ch), MILL_HERE_)
#define mill_chclose(ch) mill_chclose_((ch), MILL_HERE_)
#define mill_chs(ch, tp, val) mill_chs__((ch), tp, (val))
#define mill_chr(ch, tp) mill_chr__((ch), tp)
#define mill_chdone(ch, tp, val) mill_chdone__((ch), tp, (val))
#define mill_choose mill_choose_init__
#define mill_in(ch, tp, nm) mill_choose_in__((ch), tp, nm, __COUNTER__)
#define mill_out(ch, tp, val) mill_choose_out__((ch), tp, (val), __COUNTER__)
#define mill_deadline(dd) mill_choose_deadline__(dd, __COUNTER__)
#define mill_otherwise mill_choose_otherwise__(__COUNTER__)
#define mill_end mill_choose_end__
#else
typedef struct mill_chan_ *chan;
#define chmake(tp, sz) mill_chmake_(sizeof(tp), sz, MILL_HERE_)
#define chdup(ch) mill_chdup_((ch), MILL_HERE_)
#define chclose(ch) mill_chclose_((ch), MILL_HERE_)
#define chs(ch, tp, val) mill_chs__((ch), tp, (val))
#define chr(ch, tp) mill_chr__((ch), tp)
#define chdone(ch, tp, val) mill_chdone__((ch), tp, (val))
#define choose mill_choose_init__
#define in(ch, tp, nm) mill_choose_in__((ch), tp, nm, __COUNTER__)
#define out(ch, tp, val) mill_choose_out__((ch), tp, (val), __COUNTER__)
#define deadline(dd) mill_choose_deadline__(dd, __COUNTER__)
#define otherwise mill_choose_otherwise__(__COUNTER__)
#define end mill_choose_end__
#endif

/******************************************************************************/
/*  Debugging                                                                 */
/******************************************************************************/

/* These symbols are not wrapped in macros so that they can be used
   directly from the debugger. */
MILL_EXPORT void goredump(
    void);
MILL_EXPORT void gotrace(
    int level);

#if defined MILL_USE_PREFIX
#define mill_goredump goredump
#define mill_gotrace gotrace
#endif

#if defined(__cplusplus)
}
#endif

#endif
