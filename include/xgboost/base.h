/*!
 * Copyright (c) 2015 by Contributors
 * \file base.h
 * \brief defines configuration macros of xgboost.
 */
#ifndef XGBOOST_BASE_H_
#define XGBOOST_BASE_H_

#include <dmlc/base.h>
#include <dmlc/omp.h>
#include <cmath>
#include <iostream>

/*!
 * \brief string flag for R library, to leave hooks when needed.
 */
#ifndef XGBOOST_STRICT_R_MODE
#define XGBOOST_STRICT_R_MODE 0
#endif  // XGBOOST_STRICT_R_MODE

/*!
 * \brief Whether always log console message with time.
 *  It will display like, with timestamp appended to head of the message.
 *  "[21:47:50] 6513x126 matrix with 143286 entries loaded from
 * ../data/agaricus.txt.train"
 */
#ifndef XGBOOST_LOG_WITH_TIME
#define XGBOOST_LOG_WITH_TIME 1
#endif  // XGBOOST_LOG_WITH_TIME

/*!
 * \brief Whether customize the logger outputs.
 */
#ifndef XGBOOST_CUSTOMIZE_LOGGER
#define XGBOOST_CUSTOMIZE_LOGGER XGBOOST_STRICT_R_MODE
#endif  // XGBOOST_CUSTOMIZE_LOGGER

/*!
 * \brief Whether to customize global PRNG.
 */
#ifndef XGBOOST_CUSTOMIZE_GLOBAL_PRNG
#define XGBOOST_CUSTOMIZE_GLOBAL_PRNG XGBOOST_STRICT_R_MODE
#endif  // XGBOOST_CUSTOMIZE_GLOBAL_PRNG

/*!
 * \brief Check if alignas(*) keyword is supported. (g++ 4.8 or higher)
 */
#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || __GNUC__ > 4)
#define XGBOOST_ALIGNAS(X) alignas(X)
#else
#define XGBOOST_ALIGNAS(X)
#endif  // defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || __GNUC__ > 4)

#if defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || __GNUC__ > 4) && \
    !defined(__CUDACC__)
#include <parallel/algorithm>
#define XGBOOST_PARALLEL_SORT(X, Y, Z) __gnu_parallel::sort((X), (Y), (Z))
#define XGBOOST_PARALLEL_STABLE_SORT(X, Y, Z) \
  __gnu_parallel::stable_sort((X), (Y), (Z))
#elif defined(_MSC_VER) && (!__INTEL_COMPILER)
#include <ppl.h>
#define XGBOOST_PARALLEL_SORT(X, Y, Z) concurrency::parallel_sort((X), (Y), (Z))
#define XGBOOST_PARALLEL_STABLE_SORT(X, Y, Z) std::stable_sort((X), (Y), (Z))
#else
#define XGBOOST_PARALLEL_SORT(X, Y, Z) std::sort((X), (Y), (Z))
#define XGBOOST_PARALLEL_STABLE_SORT(X, Y, Z) std::stable_sort((X), (Y), (Z))
#endif  // GLIBC VERSION

/*!
 * \brief Tag function as usable by device
 */
#if defined (__CUDA__) || defined(__NVCC__)
#define XGBOOST_DEVICE __host__ __device__
#else
#define XGBOOST_DEVICE
#endif  // defined (__CUDA__) || defined(__NVCC__)

/*! \brief namespace of xgboost*/
namespace xgboost {
/*!
 * \brief unsigned integer type used in boost,
 *  used for feature index and row index.
 */
using bst_uint = uint32_t;  // NOLINT
using bst_int = int32_t;    // NOLINT
/*! \brief long integers */
typedef uint64_t bst_ulong;  // NOLINT(*)
/*! \brief double type, used for storing statistics */
using bst_double = double;  // NOLINT

namespace detail {
/*! \brief Implementation of gradient statistics pair. Template specialisation
 * may be used to overload different gradients types e.g. low precision, high
 * precision, integer, binary64. */
template <typename T>
class GradientPairInternal {
  /*! \brief gradient statistics */
  T grad_;
  /*! \brief second order gradient statistics */
  T hess_;

  XGBOOST_DEVICE void SetGrad(double g) { grad_ = g; }
  XGBOOST_DEVICE void SetHess(double h) { hess_ = h; }

 public:
  using ValueT = T;

  XGBOOST_DEVICE GradientPairInternal() : grad_(0), hess_(0) {}

  XGBOOST_DEVICE GradientPairInternal(double grad, double hess) {
    SetGrad(grad);
    SetHess(hess);
  }

  // Copy constructor if of same value type
  XGBOOST_DEVICE GradientPairInternal(const GradientPairInternal<T> &g)
      : grad_(g.grad_), hess_(g.hess_) {}  // NOLINT

  // Copy constructor if different value type - use getters and setters to
  // perform conversion
  template <typename T2>
  XGBOOST_DEVICE explicit GradientPairInternal(const GradientPairInternal<T2> &g) {
    SetGrad(g.GetGrad());
    SetHess(g.GetHess());
  }

  XGBOOST_DEVICE double GetGrad() const { return grad_; }
  XGBOOST_DEVICE double GetHess() const { return hess_; }

  XGBOOST_DEVICE GradientPairInternal<T> &operator+=(
      const GradientPairInternal<T> &rhs) {
    grad_ += rhs.grad_;
    hess_ += rhs.hess_;
    return *this;
  }

  XGBOOST_DEVICE GradientPairInternal<T> operator+(
      const GradientPairInternal<T> &rhs) const {
    GradientPairInternal<T> g;
    g.grad_ = grad_ + rhs.grad_;
    g.hess_ = hess_ + rhs.hess_;
    return g;
  }

  XGBOOST_DEVICE GradientPairInternal<T> &operator-=(
      const GradientPairInternal<T> &rhs) {
    grad_ -= rhs.grad_;
    hess_ -= rhs.hess_;
    return *this;
  }

  XGBOOST_DEVICE GradientPairInternal<T> operator-(
      const GradientPairInternal<T> &rhs) const {
    GradientPairInternal<T> g;
    g.grad_ = grad_ - rhs.grad_;
    g.hess_ = hess_ - rhs.hess_;
    return g;
  }

  XGBOOST_DEVICE explicit GradientPairInternal(int value) {
    *this = GradientPairInternal<T>(static_cast<double>(value),
                                  static_cast<double>(value));
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const GradientPairInternal<T> &g) {
    os << g.GetGrad() << "/" << g.GetHess();
    return os;
  }
};

template<>
inline XGBOOST_DEVICE double GradientPairInternal<int64_t>::GetGrad() const {
  return grad_ * 1e-4f;
}
template<>
inline XGBOOST_DEVICE double GradientPairInternal<int64_t>::GetHess() const {
  return hess_ * 1e-4f;
}
template<>
inline XGBOOST_DEVICE void GradientPairInternal<int64_t>::SetGrad(double g) {
  grad_ = static_cast<int64_t>(std::round(g * 1e4));
}
template<>
inline XGBOOST_DEVICE void GradientPairInternal<int64_t>::SetHess(double h) {
  hess_ = static_cast<int64_t>(std::round(h * 1e4));
}

}  // namespace detail

/*! \brief gradient statistics pair usually needed in gradient boosting */
using GradientPair = detail::GradientPairInternal<double>;

/*! \brief High precision gradient statistics pair */
using GradientPairPrecise = detail::GradientPairInternal<double>;

/*! \brief High precision gradient statistics pair with integer backed
 * storage. Operators are associative where binary64 versions are not
 * associative. */
using GradientPairInteger = detail::GradientPairInternal<int64_t>;

/*! \brief small eps gap for minimum split decision. */
const bst_double kRtEps = 1e-6f;

/*! \brief define unsigned long for openmp loop */
using omp_ulong = dmlc::omp_ulong;  // NOLINT
/*! \brief define unsigned int for openmp loop */
using bst_omp_uint = dmlc::omp_uint;  // NOLINT

/*!
 * \brief define compatible keywords in g++
 *  Used to support g++-4.6 and g++4.7
 */
#if DMLC_USE_CXX11 && defined(__GNUC__) && !defined(__clang_version__)
#if __GNUC__ == 4 && __GNUC_MINOR__ < 8
#define override
#define final
#endif  // __GNUC__ == 4 && __GNUC_MINOR__ < 8
#endif  // DMLC_USE_CXX11 && defined(__GNUC__) && !defined(__clang_version__)
}  // namespace xgboost

/* Always keep this #include at the bottom of xgboost/base.h */
#include <xgboost/build_config.h>

#endif  // XGBOOST_BASE_H_
