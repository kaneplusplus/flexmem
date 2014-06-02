# Set the memcpy offset to the SEXP header size on this system
flexmem_memcpy_offset(.Call("Rflexmem_SEXP_SIZE"))
