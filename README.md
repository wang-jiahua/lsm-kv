# KVStore using Log-structured Merge Tree

Project for SJTU SE2322 Advanced Data Structure

## Usage

These tests may require about 10 GB space. 

### Correctness Test

```sh
$ make correctness
```

### Persistence Test

```sh
$ make persistence
# kill the process following the prompt
$ make persistence -t
```

## TODO

- [ ] use bloom filter to accelerate key search
- [ ] implement range search
- [ ] add write-ahead log
