//
// Created by maxwe on 2021-06-01.
//

#ifndef JEMSYS_JEMSYS_H
#define JEMSYS_JEMSYS_H


/* ====================[ Portability Config ]================== */


#if defined(__cplusplus)
#define JEM_begin_c_namespace extern "C" {
#define JEM_end_c_namespace }

#include <cassert>

#else
#define JEM_begin_c_namespace
#define JEM_end_c_namespace

#include <assert.h>
#include <stdbool.h>
#endif

#if defined(NDEBUG)
#define JEM_debug_build false
#else
#define JEM_debug_build true
#endif

#if defined(_WIN64) || defined(_WIN32)
#define JEM_system_windows true
#define JEM_system_linux false
#define JEM_system_macos false
#if defined(_WIN64)
#define JEM_64bit true
#define JEM_32bit false
#else
#define JEM_64bit false
#define JEM_32bit true
#endif
#else
#define JEM_system_windows false
#if defined(__LP64__)
#define JEM_64bit true
#define JEM_32bit false
#else
#define JEM_64bit false
#define JEM_32bit true
#endif
#if defined(__linux__)

#define JEM_system_linux true
#define JEM_system_macos false
#elif defined(__OSX__)
#define JEM_system_linux false
#define JEM_system_macos true
#else
#define JEM_system_linux false
#define JEM_system_macos false
#endif
#endif


#if defined(_MSC_VER)
#define JEM_compiler_msvc true
#else
#define JEM_compiler_msvc false
#endif


#if defined(__clang__)
#define JEM_compiler_clang true
#else
#define JEM_compiler_clang false
#endif
#if defined(__GNUC__)
#define JEM_compiler_gcc true
#else
#define JEM_compiler_gcc false
#endif




/* =================[ Portable Attributes ]================= */



#if defined(__cplusplus)
# if JEM_compiler_msvc
#  define JEM_cplusplus _MSVC_LANG
# else
#  define JEM_cplusplus __cplusplus
# endif
# if JEM_cplusplus >= 201103L  // C++11
#  define JEM_noexcept   noexcept
# else
#  define JEM_noexcept   throw()
# endif
# if (JEM_cplusplus >= 201703)
#  define JEM_nodiscard [[nodiscard]]
# else
#  define JEM_nodiscard
# endif
#else
# define JEM_cplusplus 0
# define JEM_noexcept
# if (__GNUC__ >= 4) || defined(__clang__)
#  define JEM_nodiscard    __attribute__((warn_unused_result))
# elif (_MSC_VER >= 1700)
#  define JEM_nodiscard    _Check_return_
# else
#  define JEM_nodiscard
# endif
#endif


# if defined(__has_attribute)
#  define JEM_has_attribute(...) __has_attribute(__VA_ARGS__)
# else
#  define JEM_has_attribute(...) false
# endif


#if defined(_MSC_VER) || defined(__MINGW32__)
# if !defined(JEM_SHARED_LIB)
#  define JEM_api
# elif defined(JEM_SHARED_LIB_EXPORT)
#  define JEM_api __declspec(dllexport)
# else
#  define JEM_api __declspec(dllimport)
# endif
#
# if defined(__MINGW32__)
#  define JEM_restricted
# else
#  define JEM_restricted __declspec(restrict)
# endif
# pragma warning(disable:4127)   // suppress constant conditional warning
# define JEM_noinline          __declspec(noinline)
# define JEM_forceinline       __forceinline
# define JEM_noalias           __declspec(noalias)
# define JEM_thread_local      __declspec(thread)
# define JEM_alignas(n)        __declspec(align(n))
# define JEM_restrict          __restrict
#
# define JEM_cdecl __cdecl
# define JEM_stdcall __stdcall
# define JEM_vectorcall __vectorcall
# define JEM_may_alias
# define JEM_alloc_size(...)
# define JEM_alloc_align(p)
#
# define JEM_assume(...) __assume(__VA_ARGS__)
# define JEM_assert(expr) assert(expr)
# define JEM_unreachable JEM_assert(false); JEM_assume(false)
#
#elif defined(__GNUC__)                 // includes clang and icc
#
# if defined(__cplusplus)
# define JEM_restrict __restrict
# else
# define JEM_restrict restrict
# endif
# define JEM_cdecl                      // leads to warnings... __attribute__((cdecl))
# define JEM_stdcall
# define JEM_vectorcall
# define JEM_may_alias    __attribute__((may_alias))
# define JEM_api          __attribute__((visibility("default")))
# define JEM_restricted
# define JEM_malloc       __attribute__((malloc))
# define JEM_noinline     __attribute__((noinline))
# define JEM_noalias
# define JEM_thread_local __thread
# define JEM_alignas(n)   __attribute__((aligned(n)))
# define JEM_assume(...)
# define JEM_forceinline  __attribute__((always_inline))
# define JEM_unreachable  __builtin_unreachable()
#
# if (defined(__clang_major__) && (__clang_major__ < 4)) || (__GNUC__ < 5)
#  define JEM_alloc_size(...)
#  define JEM_alloc_align(p)
# elif defined(__INTEL_COMPILER)
#  define JEM_alloc_size(...)       __attribute__((alloc_size(__VA_ARGS__)))
#  define JEM_alloc_align(p)
# else
#  define JEM_alloc_size(...)       __attribute__((alloc_size(__VA_ARGS__)))
#  define JEM_alloc_align(p)      __attribute__((alloc_align(p)))
# endif
#else
# define JEM_restrict
# define JEM_cdecl
# define JEM_stdcall
# define JEM_vectorcall
# define JEM_api
# define JEM_may_alias
# define JEM_restricted
# define JEM_malloc
# define JEM_alloc_size(...)
# define JEM_alloc_align(p)
# define JEM_noinline
# define JEM_noalias
# define JEM_forceinline
# define JEM_thread_local            __thread        // hope for the best :-)
# define JEM_alignas(n)
# define JEM_assume(...)
# define JEM_unreachable JEM_assume(false)
#endif

#if JEM_has_attribute(transparent_union)
#define JEM_transparent_union_2(a, b) union __attribute__((transparent_union)) { a m_##a; b m_##b; }
#define JEM_transparent_union_3(a, b, c) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; }
#define JEM_transparent_union_4(a, b, c, d) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; }
#define JEM_transparent_union_5(a, b, c, d, e) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; }
#define JEM_transparent_union_6(a, b, c, d, e, f) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; }
#define JEM_transparent_union_7(a, b, c, d, e, f, g) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; }
#define JEM_transparent_union_8(a, b, c, d, e, f, g, h) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; }
#define JEM_transparent_union_9(a, b, c, d, e, f, g, h, i) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; }
#define JEM_transparent_union_10(a, b, c, d, e, f, g, h, i, j) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j;  }
#define JEM_transparent_union_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) union __attribute__((transparent_union)) { a m_##a; b m_##b; c m_##c; d m_##d; e m_##e; f m_##f; g m_##g; h m_##h; i m_##i; j m_##j; k m_##k; l m_##l; m m_##m; n m_##n; o m_##o; p m_##p; q m_##q; r m_##r; }
#else
#define JEM_transparent_union_2(a, b) void*
#define JEM_transparent_union_3(a, b, c) void*
#define JEM_transparent_union_4(a, b, c, d) void*
#define JEM_transparent_union_5(a, b, c, d, e) void*
#define JEM_transparent_union_6(a, b, c, d, e, f) void*
#define JEM_transparent_union_7(a, b, c, d, e, f, g) void*
#define JEM_transparent_union_8(a, b, c, d, e, f, g, h) void*
#define JEM_transparent_union_9(a, b, c, d, e, f, g, h, i) void*
#define JEM_transparent_union_10(a, b, c, d, e, f, g, h, i, j) void*
#define JEM_transparent_union_18(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) void*
#endif



#define JEM_no_default default: JEM_unreachable

#define JEM_cache_aligned     JEM_alignas(JEM_CACHE_LINE)
#define JEM_page_aligned      JEM_alignas(JEM_PHYSICAL_PAGE_SIZE)





/* =================[ Macro Functions ]================= */


#if JEM_compiler_gcc || JEM_compiler_clang
# define JEM_assume_aligned(p, align) __builtin_assume_aligned(p, align)
#elif JEM_compiler_msvc
# define JEM_assume_aligned(p, align) (JEM_assume(((jem_u64_t)p) % align == 0), p)
#else
# define JEM_assume_aligned(p, align) p
#endif




/* =================[ Constants ]================= */


#define JEM_DONT_CARE          ((jem_u64_t)-1)
#define JEM_NULL_HANDLE        ((void*)0)

#define JEM_WAIT               ((jem_u64_t)-1)
#define JEM_DO_NOT_WAIT        ((jem_u64_t)0)

#define JEM_CACHE_LINE         ((jem_size_t)1 << 6)
#define JEM_PHYSICAL_PAGE_SIZE ((jem_size_t)1 << 12)
#define JEM_VIRTUAL_PAGE_SIZE  ((jem_size_t)(1 << 16))



/* =================[ Types ]================= */


typedef unsigned char      jem_u8_t;
typedef   signed char      jem_i8_t;
typedef          char      jem_char_t;
typedef unsigned short     jem_u16_t;
typedef   signed short     jem_i16_t;
typedef unsigned int       jem_u32_t;
typedef   signed int       jem_i32_t;
typedef unsigned long long jem_u64_t;
typedef   signed long long jem_i64_t;



typedef size_t             jem_size_t;
typedef size_t             jem_address_t;
typedef ptrdiff_t          jem_ptrdiff_t;


typedef jem_u32_t          jem_flags32_t;
typedef jem_u64_t          jem_flags64_t;





typedef enum {
  JEM_SUCCESS,
  JEM_ERROR_INTERNAL,
  JEM_ERROR_UNKNOWN,
  JEM_ERROR_BAD_SIZE,
  JEM_ERROR_INVALID_ARGUMENT,
  JEM_ERROR_BAD_ENCODING_IN_NAME,
  JEM_ERROR_NAME_TOO_LONG,
  JEM_ERROR_INSUFFICIENT_BUFFER_SIZE,
  JEM_ERROR_BAD_ALLOC,
  JEM_ERROR_TOO_MANY_SENDERS,
  JEM_ERROR_TOO_MANY_RECEIVERS
} jem_status_t;


#endif//JEMSYS_JEMSYS_H
