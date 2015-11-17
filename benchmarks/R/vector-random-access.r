# Run with swap on.

library(microbenchmark)

# The sizes of the vectors
sizes = c(1e5, 1e6, 1e7, 1e8, 1e9, 1e10)
# The proportion of values to change
prop = 0.1

object_sizes = rep(NA, length(sizes))
bench = rep(NA, length(sizes))

# Random access for a vector 
for (i in 1:length(sizes)) {
  v = vector(mode="numeric", length=sizes[i])
  object_sizes[i] = format(object.size(v), "Gb")
  bench[i] = microbenchmark(
    {
      inds = sample.int(length(v), floor(sizes[i]*prop))
      v[inds]
    }, times=10, unit="s")$median
  
}

df = data.frame(object_size=object_size, seconds=bench, memory="xmem",
                order="random", operation="read")

# Random write. 
for (i in 1:length(sizes)) {
  v = vector(mode="numeric", length=sizes[i])
  object_sizes[i] = format(object.size(v), "Gb")
  bench[i] = summary(microbenchmark(
    {
      inds = sample.int(length(v), floor(sizes[i]*prop))
      values = as.double(sample(1:10, length(inds)))
      v[inds] = values
    }, times=10, unit="s"))$median
  
}

df = cbind(df,
           data.frame(object_size=object_size, seconds=bench, memory="RAM",
                      order="random", operation="write")
write.csv(df, "ram_bench.csv")
