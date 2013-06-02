\name{ref}
\alias{ref}
\title{Turn an R object into a reference to an R object}
\usage{
  ref(obj)
}
\arguments{
  \item{obj}{the object to turn into a reference}
}
\description{
  Turn an R object into a reference to an R object
}
\examples{
y <- ref(1:10)
print(y)
addOne <- function(x) x[] <- x[] + 1
addOne(y)
print(y)
}

