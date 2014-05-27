\name{flexmem_threshold}
\alias{flexmem_threshold}
\title{Retrieve and/or change the threshold where data structures will be
file-backed.}
\usage{
  flexmem_threshold(nbytes = 0)
}
\arguments{
  \item{nbytes}{the new threshold (in bytes)}
}
\description{
  Retrieve and/or change the threshold where data
  structures will be file-backed.
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

