#' Retrieve and/or change the threshold where data structures will be
#' file-backed.
#'
#' @param nbytes the new threshold (in bytes)
#' @seealso \code{\link{xmem_template}}
#' @export
#' @examples
#' # Add another 1000 bytes to the threshold.
#' \dontrun{
#' threshold(threshold()+1000)
#' }
threshold <- function(nbytes=0)
{
  .Call("Rflexmem_threshold", as.numeric(nbytes), PACKAGE="flexmem")
}

madvise <- function(advice=c("NORMAL","RANDOM","SEQUENTIAL"))
{
#define MADV_NORMAL     0               /* no further special treatment */
#define MADV_RANDOM     1               /* expect random page references */
#define MADV_SEQUENTIAL 2               /* expect sequential page references */
  madvise <- list(NORMAL=0,RANDOM=1,SEQUENTIAL=2)
  idx <- -1L
  if(!missing(advice)) idx <- as.integer(madvise[match.arg(advice)])
  j <- .Call("Rflexmem_madvise", idx, PACKAGE="flexmem")
  if(j>-1 && j<3) return(names(madvise)[j+1])
  j
}

memcpy_offset <- function(offset)
{
  if(missing(offset))  -1 -> offset
  .Call("Rflexmem_memcpy_offset", as.integer(offset), PACKAGE="flexmem")
}

xmem_pattern <- function(pattern="fm_XXXXXX")
{
  .Call("Rflexmem_set_pattern", as.character(pattern), PACKAGE="flexmem")
}

# Note, we use "tmp" instead of tempdir() here to correspond to default
# value in libflexmem.so.
xmem_path <- function(path="/tmp")
{
# XXX validate path first.
  .Call("Rflexmem_set_path", as.character(path), PACKAGE="flexmem")
}

xmem_get_template <- function()
{
  .Call("Rflexmem_get_template", PACKAGE="flexmem")
}

lookup <- function(object)
{
  .Call("Rflexmem_lookup", object, PACKAGE="flexmem")
}