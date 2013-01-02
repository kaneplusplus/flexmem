library(multicore)
library(flexmem)
flexmem_threshold(1e5)
x = rnorm(1e5)
print(tracemem(x))
print(flexmem_lookup(x))
cat("------------------\n")
p = fork()
if (inherits(p, "masterProcess")) {
# Here we are in the child
      cat("I'm a child! ", Sys.getpid(), "\n")
      x[2]=0
      print(head(x))
      print(tracemem(x))
      print(flexmem_lookup(x))
      rm(x)
      gc()
      exit()
}
# Here we are still in the parent.
Sys.sleep(2)
kill(p,9)

# Try modifying x and check tracemem, etc.
