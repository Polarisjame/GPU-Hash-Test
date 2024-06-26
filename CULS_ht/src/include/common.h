#pragma once

#include <cassert>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/functional.h>

#define NUM_BLOCKS(n, block_size) (((n) + (block_size) - 1) / (block_size))
#define THREAD_PER_BLOCK 32

// does not use uint64_t in cstdint since it's not supported by atomicCAS
using uint64 = unsigned long long int;
using uint32 = unsigned int;

// for static_assert false
template <class... T>
constexpr bool always_false = false;

inline int AigNodeID(int lit) {return lit >> 1;}
inline int AigNodeIsComplement(int lit) {return lit & 1;}
inline unsigned invertConstTrueFalse(unsigned lit) {
    // swap 0 and 1
    return lit < 2 ? 1 - lit : lit;
}


namespace dUtils {

__host__ __device__ __forceinline__ int AigNodeID(int lit) {return lit >> 1;}
__host__ __device__ __forceinline__ int AigNodeIsComplement(int lit) {return lit & 1;}
__host__ __device__ __forceinline__ int AigIsNode(int nodeId, int nPIs) {return nodeId > nPIs;} // considering const 1
__host__ __device__ __forceinline__ int AigIsPIConst(int nodeId, int nPIs) {return nodeId <= nPIs;}
__host__ __device__ __forceinline__ int AigNodeLitCond(int nodeId, int complement) {
    return (int)(((unsigned)nodeId << 1) | (unsigned)(complement != 0));
}
__host__ __device__ __forceinline__ int AigNodeNotCond(int lit, int complement) {
    return (int)((unsigned)lit ^ (unsigned)(complement != 0));
}
__host__ __device__ __forceinline__ int AigNodeNot(int lit) {return lit ^ 1;}
__host__ __device__ __forceinline__ int AigNodeIDDebug(int lit, int nPIs, int nPOs) {
    int id = dUtils::AigNodeID(lit);
    return dUtils::AigIsNode(id, nPIs) ? (id + nPOs) : id;
}
__host__ __device__ __forceinline__ int TruthWordNum(int nVars) {return nVars <= 5 ? 1 : (1 << (nVars - 5));}
__host__ __device__ __forceinline__ int Truth6WordNum(int nVars) {return nVars <= 6 ? 1 : (1 << (nVars - 6));}

// unary thrust functor
template <typename ValueT, typename MaskT>
struct getValueFilteredByMask {
    const MaskT maskTrue;

    getValueFilteredByMask() : maskTrue(1) {}
    getValueFilteredByMask(MaskT _maskTrue) : maskTrue(_maskTrue) {}
    
    __host__ __device__
    ValueT operator()(const thrust::tuple<ValueT, MaskT> &elem) const { 
        return thrust::get<1>(elem) == maskTrue ? thrust::get<0>(elem) : 0;
    }
};

template <typename IdT, typename LevelT>
struct decreaseLevelIds {
    __host__ __device__
    bool operator()(const thrust::tuple<IdT, LevelT> &e1, const thrust::tuple<IdT, LevelT> &e2) const {
        return thrust::get<1>(e1) == thrust::get<1>(e2) ? 
            (thrust::get<0>(e1) > thrust::get<0>(e2)) : (thrust::get<1>(e1) > thrust::get<1>(e2));
        // return thrust::get<1>(e1) > thrust::get<1>(e2);
    }
};

template <typename IdT, typename LevelT>
struct decreaseLevels {
    __host__ __device__
    bool operator()(const thrust::tuple<IdT, LevelT> &e1, const thrust::tuple<IdT, LevelT> &e2) const {
        return thrust::get<1>(e1) > thrust::get<1>(e2);
    }
};

template <typename IdT, typename LevelT>
struct decreaseLevelsPerm {
    __host__ __device__
    bool operator()(const thrust::tuple<IdT, LevelT, int> &e1, const thrust::tuple<IdT, LevelT, int> &e2) const {
        return thrust::get<1>(e1) == thrust::get<1>(e2) ? 
            (thrust::get<2>(e1) > thrust::get<2>(e2)) : (thrust::get<1>(e1) > thrust::get<1>(e2));
    }
};

template <typename T>
struct isMinusOne { 
    __host__ __device__
    bool operator()(const T &elem) {
        return elem == -1;
    }
};

template <typename T>
struct isOne { 
    __host__ __device__
    bool operator()(const T &elem) {
        return elem == 1;
    }
};

template <typename T>
struct isNotOne { 
    __host__ __device__
    bool operator()(const T &elem) {
        return elem != 1;
    }
};

template <typename T, T val>
struct equalsVal {
    __host__ __device__
    bool operator()(const T &elem) {
        return elem == val;
    }
};

template <typename T, T val>
struct notEqualsVal {
    __host__ __device__
    bool operator()(const T &elem) {
        return elem != val;
    }
};

template <typename T, T val>
struct greaterThanVal {
    __host__ __device__
    bool operator()(const T &elem) {
        return elem > val;
    }
};

template <typename T, T val>
struct greaterThanEqualsValInt {
    __host__ __device__
    int operator()(const T &elem) {
        return (elem >= val ? 1 : 0);
    }
};

template <typename T, T val>
struct lessThanVal {
    __host__ __device__
    bool operator()(const T &elem) {
        return elem < val;
    }
};

struct sameNodeID {
    __host__ __device__
    bool operator()(const int &lhs, const int &rhs) const {
        return (lhs >> 1) == (rhs >> 1);
    }
};

template <typename T>
// 判断是否是结点(不是PIs)
struct isNodeLit {
    const int nPIs;

    isNodeLit() = delete;
    isNodeLit(const int _nPIs) : nPIs(_nPIs) {}

    __host__ __device__
    bool operator()(const T &elem) {
        return (elem >> 1) > nPIs;
    }
};

template <typename T>
struct isPIConstLit {
    const int nPIs;

    isPIConstLit() = delete;
    isPIConstLit(const int _nPIs) : nPIs(_nPIs) {}

    __host__ __device__
    bool operator()(const T &elem) {
        return (elem >> 1) <= nPIs;
    }
};

struct getNodeID {
    __host__ __device__
    int operator()(const int elem) {
        return elem >> 1;
    }
};

const int AigConst1 = 0;
const int AigConst0 = 1;
} // namespace dUtils

// error checking helpers
#define cdpErrchk(ans) { cdpAssert((ans), __FILE__, __LINE__); }
__device__ __forceinline__ void cdpAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess)
   {
      printf("GPU kernel assert: %s,\nat %s, line %d\n", cudaGetErrorString(code), file, line);
      if (abort) assert(0);
   }
}

#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess) 
   {
      fprintf(stderr,"GPUassert: %s,\nat %s, line %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

#define gpuChkStackOverflow(ans) { gpuAssertStack((ans), __FILE__, __LINE__); }
inline void gpuAssertStack(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess)
   {
      fprintf(stderr,"GPUassert: %s,\nat %s, line %d\n", cudaGetErrorString(code), file, line);
      fprintf(stderr, "This is most likely due to insufficient CUDA call stack size. Try to increase cudaLimitStackSize.\n");
      if (abort) exit(code);
   }
}

// template <typename T>
// std::optional<T> get_arg_value(const std::vector<std::string>& arguments,
//                                const char* flag) {
//   uint32_t first_argument = 1;
//   for (uint32_t i = first_argument; i < arguments.size(); i++) {
//     std::string_view argument = std::string_view(arguments[i]);
//     auto key_start = argument.find_first_not_of("-");
//     auto value_start = argument.find("=");

//     bool failed = argument.length() == 0;              // there is an argument
//     failed |= key_start == std::string::npos;          // it has a -
//     failed |= value_start == std::string::npos;        // it has an =
//     failed |= key_start > 2;                           // - or -- at beginning
//     failed |= (value_start - key_start) == 0;          // there is a key
//     failed |= (argument.length() - value_start) == 1;  // = is not last

//     if (failed) {
//       std::cout << "Invalid argument: " << argument << " ignored.\n";
//       std::cout << "Use: -flag=value " << std::endl;
//       std::terminate();
//     }

//     std::string_view argument_name = argument.substr(key_start, value_start - key_start);
//     value_start++;  // ignore the =
//     std::string_view argument_value =
//         argument.substr(value_start, argument.length() - key_start);

//     if (argument_name == std::string_view(flag)) {
//       if constexpr (std::is_same<T, float>::value) {
//         return static_cast<T>(std::strtof(argument_value.data(), nullptr));
//       } else if constexpr (std::is_same<T, double>::value) {
//         return static_cast<T>(std::strtod(argument_value.data(), nullptr));
//       } else if constexpr (std::is_same<T, int>::value) {
//         return static_cast<T>(std::strtol(argument_value.data(), nullptr, 10));
//       } else if constexpr (std::is_same<T, long long>::value) {
//         return static_cast<T>(std::strtoll(argument_value.data(), nullptr, 10));
//       } else if constexpr (std::is_same<T, uint32_t>::value) {
//         return static_cast<T>(std::strtoul(argument_value.data(), nullptr, 10));
//       } else if constexpr (std::is_same<T, uint64_t>::value) {
//         return static_cast<T>(std::strtoull(argument_value.data(), nullptr, 10));
//       } else if constexpr (std::is_same<T, std::string>::value) {
//         return std::string(argument_value);
//       } else if constexpr (std::is_same<T, bool>::value) {
//         return str_tolower(argument_value) == "true";
//       } else {
//         std::cout << "Unknown type" << std::endl;
//         std::terminate();
//       }
//     }
//   }
//   return {};
// }
