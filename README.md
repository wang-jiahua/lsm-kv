# KVStore using Log-structured Merge Tree

Project for SJTU SE2322 Advanced Data Structure

## Build

```sh
mkdir build
cd build
cmake ..
make
```

## Test

The following tests may require about 10 GB space.

### Correctness Test

```sh
./correctness
```

### Persistence Test

```sh
./persistence
# kill the process following the prompt
./persistence -t
```

## TODO

- [x] use Bloom filter to accelerate key search
- [x] implement range search
- [ ] add write-ahead log
- [ ] add immutable MemTable
- [ ] redesign the structure of SSTable (integrate Bloom filter bits and metadata into files)
- [ ] use LRU cache instead of caching all indices
- [ ] use lazy recovery
