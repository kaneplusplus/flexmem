xmem
=======

```
  ___    ___ _____ ______   _______   _____ ______      
 |\  \  /  /|\   _ \  _   \|\  ___ \ |\   _ \  _   \
 \ \  \/  / | \  \\\__\ \  \ \   __/|\ \  \\\__\ \  \
  \ \    / / \ \  \\|__| \  \ \  \_|/_\ \  \\|__| \  \
   /     \/   \ \  \    \ \  \ \  \_|\ \ \  \    \ \  \
  /  /\   \    \ \__\    \ \__\ \_______\ \__\    \ \__\
 /__/ /\ __\    \|__|     \|__|\|_______|\|__|     \|__|
 |__|/ \|__|                                            

```

xmem provides utilities to override the memory allocator,
allowing users to create out-of-core data structures that may be much
larger than available RAM

Description
---

Xmem is a general, transparent tool for out-of-core (OOC) computing.
It is launched as a command line utility, taking an application as an 
argument. All memory allocations larger than a specified threshold 
are memory-mapped to a binary file. When data are not needed, they are
stored on disk. It is both process- and thread-safe.

Requirements
---

xmem requires that some version of openmp is installed on the machine.

Installing xmem
---

The xmem package can be installed by navigating to the 
`xmem/standalone/src` directory using the following commands:

```bash
mike@mike-VirtualBox:~/Projects/xmem/standalone/src
> make all
mike@mike-VirtualBox:~/Projects/xmem/standalone/src
> sudo make install
```

Note that make install will put the executable into /usr/local/bin and the
required shared object file inot /usr/local/lib.

Installing the R xmem package
---

After installing xmem the R xmem package can be installed by navigating
to the `xmem/Rpkg` directory and using the following commands in a shell:

```bash
R CMD INSTALL xmem
```

The package documentation provides more information about how to use the 
R xmem package.

Using xmem
---

Start an application in xmem by specifying the application as an argument


```r
mike@mike-VirtualBox:~/$ xmem R
```


The memory-mapped files are stored in `/tmp` by default. This parameter along
with others can be interrogated using the Rxmem package, included in
this project.

Support
---

1. xmem is supported on Linux with OS X support on the way.
2. The development home of this project can be found at: [https://github.com/kaneplusplus/xmem](https://github.com/kaneplusplus/xmem)
3. Contributions are welcome.
4. For more information contact Michael Kane at [kaneplusplus@gmail.com](kaneplusplus@gmail.com).
