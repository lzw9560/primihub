// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_DEFINES_H_
#define SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_DEFINES_H_

#include <cstdint>
#include <memory>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <string>
#include <vector>

#ifdef ENABLE_FULL_GSL
#include "src/primihub/common/gsl/span"
#else
#include "src/primihub/common/gsl/gls-lite.hpp"
#endif

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define LOCATION __FILE__ ":" STRINGIZE(__LINE__)
#define RTE_LOC std::runtime_error(LOCATION)

#ifdef _MSC_VER
    #ifndef _WIN32_WINNT
        // compile for win 7 and up.
        #define _WIN32_WINNT 0x0601
    #endif
  #pragma warning(disable : 4018)
  // signed unsigned comparison warning
  #define TODO(x) __pragma(message (__FILE__ ":" STRINGIZE(__LINE__) " Warning:TODO - " #x))
#else
  #define TODO(x)
#endif

// add instrinsics names that intel knows but clang doesn't…
#ifdef __clang__
#define _mm_cvtsi128_si64x _mm_cvtsi128_si64
#endif

namespace primihub {

  template<typename T> using ptr = T*;
  template<typename T> using uPtr = std::unique_ptr<T>;
  template<typename T> using sPtr = std::shared_ptr<T>;
  template<typename T> using span = gsl::span<T>;

  typedef uint64_t u64;
  typedef int64_t i64;
  typedef uint32_t u32;
  typedef int32_t i32;
  typedef uint16_t u16;
  typedef int16_t i16;
  typedef uint8_t u8;
  typedef int8_t i8;

  constexpr u64 divCeil(u64 val, u64 d) { return (val + d - 1) / d; }
  constexpr u64 roundUpTo(u64 val, u64 step) {
    return divCeil(val, step) * step;
  }

  u64 log2ceil(u64);
  u64 log2floor(u64);

  static inline uint64_t mod64(uint64_t word, uint64_t p) {
#ifdef __SIZEOF_INT128__
    return (uint64_t)(((__uint128_t)word * (__uint128_t)p) >> 64);
#elif defined(_MSC_VER) && defined(_WIN64)
    uint64_t highProduct;
    _umul128(word, p, &highProduct);
    return highProduct;
    unsigned __int64 _umul128(
      unsigned __int64 Multiplier,
      unsigned __int64 Multiplicand,
      unsigned __int64* HighProduct);
#else
    return word % p;
#endif
  }


enum class Channel_Status { Normal, Closing, Closed, Canceling};

enum class Errc_Status {
  success = 0
};

enum class retcode {
  SUCCESS = 0,
  FAIL,
};

}

#endif  // SRC_primihub_UTIL_NETWORK_SOCKET_COMMON_DEFINES_H_
