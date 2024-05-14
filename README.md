## Evaluating Inserton & Find Rate of AtomicCAS GPU Hash and BGHT [reference](https://github.com/owensgroup/BGHT)

## Compile

+ BGHT: `./rebuild.sh`
+ atomicCAS: 
  ```sh
    cd CULS_ht
    mkdir build && cd build
    cmake ..
    make
    cd ../..
  ```

## Evaluate

+ BGHT: 
  ```sh
  cd build/bin
  ./bcht_example -num-keys [num of keys to insert] -loadfactor [float, load factor of hash table]
  ```
+ atomicCAS:
  1. move cache file generated in BGHT(/build/bin/dataset) to /CULS_ht/build/dataset
  2. ```sh
      cd /CULS_ht/build/
      ./ghash -num-keys -loadfactor
      ```
