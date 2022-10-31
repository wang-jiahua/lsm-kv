# LSM-KV

KVStore implemented in Log-structured Merge Tree, based on SJTU SE2322 Advanced Data Structure.

## Build and Test

```sh
cmake -DCMAKE_BUILD_TYPE=Release -Bbuild
cd build
cmake --build .
ctest
```

## TODO

- [x] use Bloom filter to accelerate key search
- [x] implement range search
- [ ] add write-ahead log
- [ ] add immutable MemTable
- [ ] redesign the structure of SSTable (integrate Bloom filter bits and metadata into files)
- [ ] use LRU cache instead of caching all indices
- [ ] use lazy recovery
