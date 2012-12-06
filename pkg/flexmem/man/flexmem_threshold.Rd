\name{flexmem_threshold}
\alias{flexmem_threshold}
\title{Retrieve or change the threshold where data structures will be file-backed.}
\usage{
  flexmem_threshold(nbytes = 1000)
}
\arguments{
  \item{nbytes}{the new threshold (in bytes)}
}
\description{
  Retrieve or change the threshold where data structures
  will be file-backed.
}
\examples{
# Add another 1000 bytes to the threshold.
\dontrun{
flexmem_threshold(flexmem_threshold()+1000)
}
}
\seealso{
  \code{\link{flexmem_template}}
}

