
.onLoad <- function(libname, pkgname) {
  library.dynam("flexmem", pkgname, libname)
}

.onUnload <- function(libpath) {
  library.dynam.unload("flexmem", libpath)
}
