# KVStore using Log-structured Merge Tree

Project for SJTU SE2322 Advanced Data Structure

## Build

```sh
~/lsm-kv$ mkdir build
~/lsm-kv$ cd build
~/lsm-kv/build$ cmake ..
~/lsm-kv/build$ make
```

## Test

The following tests may require about 10 GB space. 

### Correctness Test

```sh
~/lsm-kv/build$ make correctness
~/lsm-kv/build$ rm -rf data
```

### Persistence Test

```sh
~/lsm-kv/build$ make persistence
# kill the process following the prompt
~/lsm-kv/build$ make persistence -t
~/lsm-kv/build$ rm -rf data
```

## TODO

- [ ] use bloom filter to accelerate key search
- [ ] implement range search
- [ ] add write-ahead log
