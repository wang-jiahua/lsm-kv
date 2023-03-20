# LSM-KV

Key-Value Storage implemented in Log-structured Merge Tree.

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
- [x] add write-ahead wal
- [x] add immutable MemTable
- [ ] redesign the structure of SSTable (integrate Bloom filter bits and metadata into files)
- [ ] flush periodically
