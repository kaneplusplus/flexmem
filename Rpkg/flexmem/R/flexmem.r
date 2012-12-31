
#' Retrieve and/or change the threshold where data structures will be
#' file-backed.
#'
#' @param nbytes the new threshold (in bytes)
#' @seealso \code{\link{flexmem_template}}
#' @export
#' @examples
#' # Add another 1000 bytes to the threshold.
#' \dontrun{
#' flexmem_threshold(flexmem_threshold()+1000)
#' }
flexmem_threshold <- function(nbytes=1e9)
{
  .Call("Rflexmem_threshold", as.numeric(nbytes), PACKAGE="flexmem")
}

flexmem_set_pattern <- function(pattern="fm_XXXXXX")
{
  .Call("Rflexmem_set_pattern", as.character(pattern), PACKAGE="flexmem")
}

# Note, we use "tmp" instead of tempdir() here to correspond to default
# value in libflexmem.so.
flexmem_set_path <- function(path="/tmp")
{
# XXX validate path first.
  .Call("Rflexmem_set_path", as.character(path), PACKAGE="flexmem")
}

flexmem_get_template <- function()
{
  .Call("Rflexmem_get_template", PACKAGE="flexmem")
}

flexmem_lookup <- function(object)
{
  .Call("Rflexmem_lookup", object, PACKAGE="flexmem")
}


#' The R5 Reference class
#' 
#' Pass an R object to ref. The return is an object that behaves like the 
#' original object but exhibits reference semantics. This class is useful
#' when dealing with large objects and a user want to have control over when
#' the object is copied.
#' @export  
Reference <- setRefClass("Reference",
  fields=list(obj="ANY")
)

#' Turn an R object into a reference to an R object
#'
#' @param obj the object to turn into a reference
#' @export
#' @examples
#' y <- ref(1:10)
#' print(y)
#' addOne <- function(x) x[] <- x[] + 1
#' addOne(y)
#' print(y)
ref <- function(obj) {
  fm <- Reference$new(obj=obj)
}

# Bracket operator for a 1-d container.

#' @name [
#' @aliases [,Reference,ANY-method [,Reference,ANY,character-method, [,Reference,ANY-logical-method [,Reference,ANY,numeric-method [,Reference,ANY,ANY-method [,Reference,ANY,character-method [,Reference,ANY,logical-method [,Reference,ANY,numeric-method [[,Reference,ANY,ANY-method [[,Reference,ANY,character-method [[,Reference,ANY,logical-method [[,Reference,ANY,numeric-method [<-,Reference,ANY,ANY-method [<-,Reference,ANY,character-method [<-,Reference,ANY,logical-method [<-,Reference,ANY,numeric-method [[<-,Reference,ANY,ANY-method [[<-,Reference,ANY,character-method [[<-,Reference,ANY,logical-method [[<-,Reference,ANY,numeric-method
#' @docType methods
#' @rdname extract-methods
#' @title Extract or replace values from a Reference object
#' @description These functions extract values from Refernce objects 
#' holding n-dimensional data structures.
#' @export
setMethod("[",
  signature(x="Reference", i="ANY"),
  function(x, i, drop=TRUE) {
    x$obj[i=i, drop=drop]
  })

# Double-bracket operator for a 1-d container.

#' @export
setMethod("[[",
  signature(x="Reference", i="ANY"),
  function(x, i, exact=TRUE) {
    x$obj[[i=i, exact=exact]]
  })

# Bracket operators for a 2-d containers.

#' @export
setMethod("[",
  signature(x="Reference", i="ANY", j="numeric"),
  function(x, i, j, ..., drop=TRUE) {
    x$obj[i=i, j=j, ...=..., drop=drop]
  })

#' @export
setMethod("[",
  signature(x="Reference", i="ANY", j="logical"),
  function(x, i, j, ..., drop=TRUE) {
    x$obj[i=i, j=j, ...=..., drop=drop]
  })

#' @export
setMethod("[",
  signature(x="Reference", i="ANY", j="character"),
  function(x, i, j, ..., drop=TRUE) {
    x$obj[i=i, j=j, ...=..., drop=drop]
  })


# Double-bracket operator for a 2-d container.

#' @export
setMethod("[[",
  signature(x="Reference", i="ANY", j="numeric"),
  function(x, i, j, ..., exact=TRUE) {
    x$obj[[i=i, j=j, ..., exact=exact]]
  })

#' @export
setMethod("[[",
  signature(x="Reference", i="ANY", j="logical"),
  function(x, i, j, ..., exact=TRUE) {
    x$obj[[i=i, j=j, ...=..., exact=exact]]
  })

#' @export
setMethod("[[",
  signature(x="Reference", i="ANY", j="character"),
  function(x, i, j, ..., exact=TRUE) {
    x$obj[[i=i, j=j, ...=..., exact=exact]]
  })

# Bracket assignment oeprator for 1-d container

#' @export
setMethod("[<-",
  signature(x="Reference", i="ANY"),
  function(x, i, value) {
    x$obj[i=i] <- value
    x
  })

# Double-bracket assignment operator for a 1-d container.

#' @export
setMethod("[[<-",
  signature(x="Reference", i="ANY"),
  function(x, i, value) {
    x$obj[[i=i]] <- value
    x
  })

# Bracket assignment operator for a 2-d container.

#' @export
setMethod("[<-",
  signature(x="Reference", i="ANY", j="numeric"),
  function(x, i, j, ..., value) {
    x$obj[i=i, j=j, ...=...] <- value
    x
  })

#' @export
setMethod("[<-",
  signature(x="Reference", i="ANY", j="logical"),
  function(x, i, j, ..., value) {
    x$obj[i=i, j=j, ...=...,] <- value
    x
  })

#' @export
setMethod("[<-",
  signature(x="Reference", i="ANY", j="character"),
  function(x, i, j, ..., value) {
    x$obj[i=i, j=j, ...=...,] <- value
    x
  })

# Double-bracket assignment operator for a 2-d container.

#' @export
setMethod("[[<-",
  signature(x="Reference", i="ANY", j="numeric"),
  function(x, i, j, ..., value) {
    x[[i=i, j=j, ...=...]] <- value
    x
  })

#' @export
setMethod("[[<-",
  signature(x="Reference", i="ANY", j="logical"),
  function(x, i, j, ..., value) {
    x[[i=i, j=j, ...=...]] <- value
    x
  })

#' @export
setMethod("[[<-",
  signature(x="Reference", i="ANY", j="character"),
  function(x, i, j, ..., value) {
    x[[i=i, j=j, ...=...]] <- value
    x
  })

#' @name names
#' @aliases names,Reference-method names<-,Reference-method
#' @title Extract or replace the names of an object held by a Reference object.
#' @export 
setMethod("names",
  signature(x="Reference"),
  function(x) {
    names(x$obj)
  })

#' @export
setMethod("names<-",
  signature(x="Reference"),
  function(x, value) {
    names(x$obj) <- value
    x
  })

#' @name length
#' @aliases length,Reference-method length<-,Reference-method
#' @title Get or change the length of an object held by a Reference object.
#' @export 
setMethod("length",
  signature(x="Reference"),
  function(x) {
    length(x$obj)
  })

#' @export
setMethod("length<-",
  signature(x="Reference"),
  function(x, value) {
    length(x$obj) <- value
    x
  })

#' @name dimanmes
#' @aliases dimnames,Reference-method dimnames<-,Reference-method
#' @title Get or change the dimension names of an object held by a Reference 
#' class instance.
#' @export 
setMethod("dimnames",
  signature(x="Reference"),
  function(x) {
    dimnames(x$obj)
  })

#' @export
setMethod("dimnames<-",
  signature(x="Reference"),
  function(x, value) {
    dimnames(x$obj) <- value
    x
  })

# Math operations.

#' @name +
#' @aliases %%,ANY,Reference-method %%,Reference,ANY-method %*%,ANY,Reference-method %*%,Reference,ANY-method *,ANY,Reference-method *,Reference,ANY-method +,ANY,Reference-method +,Reference,ANY-method -,ANY,Reference-method -,Reference,ANY-method /,ANY,Reference-method /,Reference,ANY-method
#' @rdname arithmetic-operators
#' @title Perform arithmetic operations with objects held by a Reference class 
#' instance.
#' @export
setMethod("+",
  signature(e1="Reference"),
  function(e1, e2) {
    ref(e1$obj + e2)
  })

#' @export
setMethod("+",
  signature(e2="Reference"),
  function(e1, e2) {
    ref(e1 + e2$obj)
  })
  
#' @export
setMethod("-",
  signature(e1="Reference"),
  function(e1, e2) {
    ref(e1$obj - e2)
  })

#' @export
setMethod("-",
  signature(e2="Reference"),
  function(e1, e2) {
    ref(e1 - e2$obj)
  })
  
#' @export
setMethod("/",
  signature(e1="Reference"),
  function(e1, e2) {
    ref(e1$obj / e2)
  })

#' @export
setMethod("/",
  signature(e2="Reference"),
  function(e1, e2) {
    ref(e1 / e2$obj)
  })
  
#' @export
setMethod("*",
  signature(e1="Reference"),
  function(e1, e2) {
    ref(e1$obj * e2)
  })

#' @export
setMethod("*",
  signature(e2="Reference"),
  function(e1, e2) {
    ref(e1 * e2$obj)
  })
  
#' @export
setMethod("%%",
  signature(e1="Reference"),
  function(e1, e2) {
    ref(e1$obj %% e2)
  })

#' @export
setMethod("%%",
  signature(e2="Reference"),
  function(e1, e2) {
    ref(e1 %% e2$obj)
  })
  
#' @export
setMethod("%*%",
  signature(x="Reference"),
  function(x, y) {
    ref(x$obj %*% y)
  })

#' @export
setMethod("%*%",
  signature(y="Reference"),
  function(x, y) {
    ref(x %*% y$obj)
  })

#' @name crossprod
#' @aliases crossprod,ANY,Reference-method crossprod,Reference,ANY-method 
#' @title Get the matrix cross product when one of the matrices is maintained
#' by a Reference class instance.
#' @export
setMethod("crossprod",
  signature(x="Reference"),
  function(x, y) {
    ref(crossprod(x$obj, y))
  })

#' @export
setMethod("crossprod",
  signature(y="Reference"),
  function(x, y) {
    ref(crossprod(x, y$obj))
  })

#' @name t
#' @aliases t,Reference-method
#' @title Take the transpose of an object maintained by a Reference class 
#' object.
#' @export
setMethod("t",
  signature(x="Reference"),
  function(x) {
    ref(t(x$obj))
  })

#' @name solve
#' @aliases solve,ANY,Reference-method solve,Reference,ANY-method 
#' @title Solve a system of linear equations when one of the matrices is 
#' maintained by a Reference class instance.
#' @export
setMethod("solve",
  signature(a="Reference"),
  function(a, b, ...) {
    ref(solve(a=a, b=b, ...=...))
  })

#' @export
setMethod("solve",
  signature(b="Reference"),
  function(a, b, ...) {
    ref(solve(a=a, b=b, ...=...))
  })
  
