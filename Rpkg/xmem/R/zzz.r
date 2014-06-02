.onLoad = function(libname,pkgname)
{
# Set the memcpy offset to the SEXP header size on this system
  tryCatch(
    memcpy_offset(.Call("Rxmem_SEXP_SIZE", PACKAGE="xmem")),
    error=function(e) cat("Xmem is not available. Please invoke R with xmem before loading this package.\n",file=stderr())
  )
}
