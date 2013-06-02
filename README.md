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

Requirements
---

flexmem requires that some version of openmp is installed on the machine.

Installing flexmem
---

The flexmem package can be installed by navigating to the 
`flexmem/standalone/src` directory using the following commands:

```bash
mike@mike-VirtualBox:~/Projects/flexmem/standalone/src
> make all
mike@mike-VirtualBox:~/Projects/flexmem/standalone/src
> sudo make install
```

Please note that you may get a warning message concerning "__malloc_hook".
This is due to a recent change in gcc and is currently being worked through.
Also note that install will put the executable into /usr/local/bin and the
required shared object file inot /usr/local/lib.

Installing the R flexmem package
---

After installing flexmem the R flexmem package can be installed by navigating
to the `flexmem/Rpkg` directory and using the following commands in a shell:

```bash
R CMD INSTALL flexmem
```

The package documentation provides more information about how to use the 
R flexmem package.

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
