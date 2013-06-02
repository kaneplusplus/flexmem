flexmem
=======

flexmem provides utilities to override the memory allocator,
allowing users to create out-of-core data structures that may be much
larger than available RAM

Description
---

Flexmem is a general, transparent tool for out-of-core (OOC) computing.
It is launched as a command line utility, taking an application as an 
argument. All memory allocations larger than a specified threshold 
are memory-mapped to a binary file. When data are not needed, they are
stored on disk. It is both process- and thread-safe.

Using flexmem
---

Start an application in flexmem by specifying the application as an argument


```r
mike@mike-VirtualBox:~/$ flexmem R
```


The memory-mapped files are stored in `/tmp` by default. This parameter along
with others can be interrogated using the Rflexmem package, included in
this project.

Support
---

1. flexmem is supported on Linux with OS X support on the way.
2. The development home of this project can be found at: [https://github.com/kaneplusplus/flexmem](https://github.com/kaneplusplus/flexmem)
3. Contributions are welcome.
4. For more information contact Michael Kane at [kaneplusplus@gmail.com](kaneplusplus@gmail.com).
