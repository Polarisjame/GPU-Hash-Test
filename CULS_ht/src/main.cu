#include "strash.h"
#include "cmd.hpp"

// #define THREAD_PER_BLOCK 16
#define NUM_BLOCKS(n, block_size) (((n) + (block_size) - 1) / (block_size))

int main(int argc, char **argv){
    using key_type = uint32_t;
    using value_type = uint32_t;

    auto arguments = std::vector<std::string>(argv, argv + argc);
    std::size_t num_keys =
        get_arg_value<std::size_t>(arguments, "num-keys").value_or(16ull);
    double load_factor = get_arg_value<double>(arguments, "load-factor").value_or(0.9);

    std::size_t capacity = num_keys/load_factor;
    key_type *keys;
    value_type *values;

    int a = clkAndEvaluate(num_keys, keys, values, capacity);
    
    return 0;
}