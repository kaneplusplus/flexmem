\name{flexmem_template}
\alias{flexmem_template}
\title{Retrieve or change the template for temporary files in flexmem.}
\usage{
  flexmem_template(newTemplate = "./tmp/FlexMemXXXXXX")
}
\arguments{
  \item{newTemplate}{the new template}
}
\description{
  Retrieve or change the template for temporary files in
  flexmem.
}
\examples{
# Change the template to create files in the current directory.
\dontrun{
flexmem_template("./")
}
}
\seealso{
  \code{\link{flexmem_threshold}}
}

