# Run with xmem R

library(xmem)
library(microbenchmark)

# The sizes of the vectors
sizes = rev(seq(0.1, 13, length.out=5)*1e8)
# The proportion of values to change
prop = 0.01

object_size = rep(NA, length(sizes))
bench = rep(NA, length(sizes))

# Random access for a vector 
for (i in 1:length(sizes)) {
  print(i)
  v = vector(mode="numeric", length=sizes[i])
  object_size[i] = format(object.size(v), "Gb")
  print(object.size(v), units="Gb")
  bench[i] = summary(microbenchmark(
    {
      inds = sample.int(length(v), floor(sizes[i]*prop), replace=TRUE)
      v[inds]
    }, times=1, unit="s"))$mean
  print("Calling gc")
  gc()
  print(bench[i])
}

df = data.frame(object_size=object_size, seconds=bench, memory="xmem",
                order="random", operation="read")

# random write. 
for (i in 1:length(sizes)) {
  print(i)
  v = vector(mode="numeric", length=sizes[i])
  object_size[i] = format(object.size(v), "Gb")
  bench[i] = summary(microbenchmark(
    {
      inds = sample.int(length(v), floor(sizes[i]*prop), replace=TRUE)
      values = as.double(sample(1:10, length(inds), replace=TRUE))
      v[inds] = values
    }, times=1, unit="s"))$median
  gc()
  print(bench[i])
}

df = cbind(df,
           data.frame(object_size=object_size, seconds=bench, memory="xmem",
                      order="random", operation="write"))
write.csv(df, "xmem_bench.csv")
