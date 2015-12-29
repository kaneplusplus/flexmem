# Run with xmem R

library(xmem)
library(microbenchmark)

# The sizes of the vectors
sizes = seq(0.1, 20, length.out=5)*1e8
# The proportion of values to change
prop = 0.01

object_size = rep(NA, length(sizes))
bench = rep(NA, length(sizes))

print("read")
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
  print(object_size[i])
  print(bench[i])
}

df = data.frame(object_size=object_size, seconds=bench, memory="xmem",
                order="random", operation="read")

print("write")
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
  print(object_size[i])
  print(bench[i])
}

df = cbind(df,
           data.frame(object_size=object_size, seconds=bench, memory="xmem",
                      order="random", operation="write"))
write.csv(df, "mem_bench.csv")
