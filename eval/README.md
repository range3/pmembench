- [pmem2bench](#pmem2bench)
  - [Devdax sequential nt-store/noflush load access comparison](#devdax-sequential-nt-storenoflush-load-access-comparison)
  - [devdax write](#devdax-write)
  - [devdax read (noflush)](#devdax-read-noflush)

# pmem2bench
## Devdax sequential nt-store/noflush load access comparison

| write                                               | read                                               |
| :-------------------------------------------------- | :------------------------------------------------- |
| ![](fig/pmem2bench/pmembench-comp-devdax-write.png) | ![](fig/pmem2bench/pmembench-comp-devdax-read.png) |

## devdax write

|         | Pegasus (Optane 300)                                                             | chris90 (Optane 200)                                                             | chris80 (Optane 100)                                                             |
| :------ | :------------------------------------------------------------------------------- | :------------------------------------------------------------------------------- | :------------------------------------------------------------------------------- |
| seq-nt  | ![](fig/pmem2bench/pmembench-pegasus-libpmem2-devdax-write-sequential-movnt.png) | ![](fig/pmem2bench/pmembench-chris90-libpmem2-devdax-write-sequential-movnt.png) | ![](fig/pmem2bench/pmembench-chris80-libpmem2-devdax-write-sequential-movnt.png) |
| seq-t   | ![](fig/pmem2bench/pmembench-pegasus-libpmem2-devdax-write-sequential.png)       | ![](fig/pmem2bench/pmembench-chris90-libpmem2-devdax-write-sequential.png)       | ![](fig/pmem2bench/pmembench-chris80-libpmem2-devdax-write-sequential.png)       |
| rand-nt | ![](fig/pmem2bench/pmembench-pegasus-libpmem2-devdax-write-random-movnt.png)     | ![](fig/pmem2bench/pmembench-chris90-libpmem2-devdax-write-random-movnt.png)     | ![](fig/pmem2bench/pmembench-chris80-libpmem2-devdax-write-random-movnt.png)     |
| rand-t  | ![](fig/pmem2bench/pmembench-pegasus-libpmem2-devdax-write-random.png)           | ![](fig/pmem2bench/pmembench-chris90-libpmem2-devdax-write-random.png)           | ![](fig/pmem2bench/pmembench-chris80-libpmem2-devdax-write-random.png)           |

## devdax read (noflush)

|               | Pegasus (Optane 300)                                                              | chris90 (Optane 200)                                                              | chris80 (Optane 100)                                                              |
| :------------ | :-------------------------------------------------------------------------------- | :-------------------------------------------------------------------------------- | :-------------------------------------------------------------------------------- |
| seq-libpmem2  | ![](fig/pmem2bench/pmembench-pegasus-libpmem2-devdax-read-sequential-noflush.png) | ![](fig/pmem2bench/pmembench-chris90-libpmem2-devdax-read-sequential-noflush.png) | ![](fig/pmem2bench/pmembench-chris80-libpmem2-devdax-read-sequential-noflush.png) |
| seq-glibc     | ![](fig/pmem2bench/pmembench-pegasus-libc-devdax-read-sequential-noflush.png)     | ![](fig/pmem2bench/pmembench-chris90-libc-devdax-read-sequential-noflush.png)     | ![](fig/pmem2bench/pmembench-chris80-libc-devdax-read-sequential-noflush.png)     |
| rand-libpmem2 | ![](fig/pmem2bench/pmembench-pegasus-libpmem2-devdax-read-random-noflush.png)     | ![](fig/pmem2bench/pmembench-chris90-libpmem2-devdax-read-random-noflush.png)     | ![](fig/pmem2bench/pmembench-chris80-libpmem2-devdax-read-random-noflush.png)     |
| rand-glibc    | ![](fig/pmem2bench/pmembench-pegasus-libc-devdax-read-random-noflush.png)         | ![](fig/pmem2bench/pmembench-chris90-libc-devdax-read-random-noflush.png)         | ![](fig/pmem2bench/pmembench-chris80-libc-devdax-read-random-noflush.png)         |
