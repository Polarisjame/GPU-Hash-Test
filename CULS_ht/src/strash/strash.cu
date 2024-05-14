#include "strash.h"
#include "gpu_timer.hpp"
#include "hash_table.h"

template <typename KeyT, typename ValueT>
ValueT generate_value(KeyT in) {
  return in + 1;
}

template <typename KeyT, typename ValueT, typename size_type>
__host__ void generate_uniform_unique_pairs(KeyT *keys,
                               ValueT *values,
                               size_type num_keys,
                               bool cache=false){
    std::cout << "Begin Gen: " << num_keys << std::endl;
    unsigned min_key = 0;
    unsigned seed = 1;
    // bool cache = true;
    std::string dataset_dir = "dataset";
    std::string dataset_name = std::to_string(num_keys) + "_" + std::to_string(seed);
    std::string dataset_path = dataset_dir + "/" + dataset_name;
    if (cache) {
        if (std::filesystem::exists(dataset_dir)) {
        if (std::filesystem::exists(dataset_path)) {
            std::cout << "Reading cached keys.." << std::endl;
            std::ifstream dataset(dataset_path, std::ios::binary);
            dataset.read((char*)keys, sizeof(KeyT) * num_keys);
            dataset.read((char*)values, sizeof(ValueT) * num_keys);
            dataset.close();
            return;
        }
        } else {
        std::filesystem::create_directory(dataset_dir);
        }
    }
    std::random_device rd;
    std::mt19937 rng(seed); //生成伪随机数引擎, 种子为seed
    auto max_key = std::numeric_limits<KeyT>::max() - 1; 
    std::uniform_int_distribution<KeyT> uni(min_key, max_key); //生成区间为min-max随机数字的对象
    // std::unordered_set<key_type> unique_keys;
    int count = 0;
    std::cout << "Start Random Gen" << std::endl;
    while (count < num_keys) {
        // unique_keys.insert(uni(rng)); //随机生成范围内数字插入
        // unique_keys.insert(unique_keys.size() + 1);
        keys[count++] = uni(rng);
    }
    // std::copy(unique_keys.cbegin(), unique_keys.cend(), keys);
    // std::shuffle(keys.begin(), keys.end(), rng);

    #ifdef _WIN32
    // OpenMP + windows don't allow unsigned loops
    for (uint32_t i = 0; i < unique_keys.size(); i++) {
        values[i] = generate_value<key_type, value_type>(keys[i]);
    }
    #else

    for (uint32_t i = 0; i < num_keys; i++) {
        values[i] = generate_value<KeyT,ValueT>(keys[i]);
    }
    #endif

    if (cache) {
        std::cout << "Caching.." << std::endl;
        std::ofstream dataset(dataset_path, std::ios::binary);
        dataset.write((char*)keys, sizeof(KeyT) * num_keys);
        dataset.write((char*)values, sizeof(ValueT) * num_keys);
        dataset.close();
    }
    std::cout << "Finish Random Gen" << std::endl;
}

template <typename KeyT, typename ValueT, typename size_type>
__global__ void insert_batch_kvpairs(size_type num_keys, KeyT *ht_keys, ValueT *ht_values, KeyT *keys, ValueT *values, size_type capacity){
    size_type idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx < num_keys){
        insert_single_no_update(ht_keys, ht_values, keys[idx], values[idx], capacity);
    }
}

template <typename KeyT, typename ValueT, typename size_type>
__global__ void retriev_batch_kvpairs(size_type num_keys, KeyT *ht_keys, ValueT *ht_values, KeyT *keys, ValueT *values, size_type capacity){
    size_type idx = blockIdx.x * blockDim.x + threadIdx.x;
    ValueT temp;
    if(idx < num_keys){
        temp = retrieve_single(ht_keys, ht_values, keys[idx], capacity);
        if (temp != values[idx]){
            printf("Find Error!\n");
        }
    }
}

int clkAndEvaluate(uint32_t num_keys, uint32_t *keys, uint32_t *values, uint32_t capacity){
    uint32_t *g_keys;
    uint32_t *g_values;
    HashTable<uint32_t, uint32_t> ht_table(capacity);
    uint32_t* ht_keys = ht_table.get_keys_storage();
    uint32_t* ht_values = ht_table.get_values_storage();
    keys = (uint32_t*)malloc(num_keys*sizeof(uint32_t));
    values = (uint32_t*)malloc(num_keys*sizeof(uint32_t));
    generate_uniform_unique_pairs(keys, values, num_keys, true);
    // for (uint32_t i = num_keys - 100; i < num_keys; i++) std::cout<< i << ": " << keys[i] << std::endl;
    cudaMalloc(&g_keys, num_keys*sizeof(uint32_t));
    cudaMalloc(&g_values, num_keys*sizeof(uint32_t));
    cudaMemcpy(g_keys, keys, num_keys*sizeof(uint32_t), cudaMemcpyHostToDevice);
    cudaMemcpy(g_values, values, num_keys*sizeof(uint32_t), cudaMemcpyHostToDevice);
    cudaStream_t stream;
    cudaStreamCreate(&stream);
    gpu_timer timer(stream);
    timer.start_timer();
    insert_batch_kvpairs<<<NUM_BLOCKS(num_keys, THREAD_PER_BLOCK), THREAD_PER_BLOCK, 0, stream>>>(num_keys, ht_keys, ht_values, g_keys, g_values, capacity);
    // cudaDeviceSynchronize();
    timer.stop_timer();
    auto insert_s = timer.get_elapsed_s();
    printf("Finished Hash, time = %lf secs\n", insert_s);
    // clock_t start = clock();
    // clock_t finish = clock();
    // printf("Finished Hash, time = %lf secs\n", (finish - start) / (double) CLOCKS_PER_SEC);
    timer.start_timer();
    retriev_batch_kvpairs<<<NUM_BLOCKS(num_keys, THREAD_PER_BLOCK), THREAD_PER_BLOCK, 0, stream>>>(num_keys, ht_keys, ht_values, g_keys, g_values, capacity);
    // cudaDeviceSynchronize();
    timer.stop_timer();
    auto find_s = timer.get_elapsed_s();
    // start = clock();
    // retriev_batch_kvpairs<<<NUM_BLOCKS(num_keys, THREAD_PER_BLOCK), THREAD_PER_BLOCK>>>(num_keys, ht_keys, ht_values, g_keys, g_values, capacity);
    // cudaDeviceSynchronize();
    // finish = clock();
    printf("Finished Find, time = %lf secs\n", find_s);
    return 0;
}