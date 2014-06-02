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
  .Call("Rxmem_threshold", as.numeric(nbytes), PACKAGE="xmem")
}

madvise <- function(advice=c("NORMAL","RANDOM","SEQUENTIAL"))
{
#define MADV_NORMAL     0               /* no further special treatment */
#define MADV_RANDOM     1               /* expect random page references */
#define MADV_SEQUENTIAL 2               /* expect sequential page references */
  madvise <- list(NORMAL=0,RANDOM=1,SEQUENTIAL=2)
  idx <- -1L
  if(!missing(advice)) idx <- as.integer(madvise[match.arg(advice)])
  j <- .Call("Rxmem_madvise", idx, PACKAGE="xmem")
  if(j>-1 && j<3) return(names(madvise)[j+1])
  j
}

memcpy_offset <- function(offset)
{
  if(missing(offset))  -1 -> offset
  .Call("Rxmem_memcpy_offset", as.integer(offset), PACKAGE="xmem")
}

xmem_pattern <- function(pattern="fm_XXXXXX")
{
  .Call("Rxmem_set_pattern", as.character(pattern), PACKAGE="xmem")
}

# Note, we use "tmp" instead of tempdir() here to correspond to default
# value in libxmem.so.
xmem_path <- function(path="/tmp")
{
# XXX validate path first.
  .Call("Rxmem_set_path", as.character(path), PACKAGE="xmem")
}

xmem_get_template <- function()
{
  .Call("Rxmem_get_template", PACKAGE="xmem")
}

lookup <- function(object)
{
  .Call("Rxmem_lookup", object, PACKAGE="xmem")
}
