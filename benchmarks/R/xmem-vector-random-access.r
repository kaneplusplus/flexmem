# Run with xmem R

library(xmem)
library(microbenchmark)

# The sizes of the vectors
sizes = seq(0.1, 20, length.out=10)*1e8
# The proportion of values to change
prop = 0.1

object_size = rep(NA, length(sizes))
bench = rep(NA, length(sizes))

# Random access for a vector 
for (i in 1:length(sizes)) {
  print(i)
  v = vector(mode="numeric", length=sizes[i])
  object_size[i] = format(object.size(v), "Gb")
  print(object.size(v), units="Gb")
  inds = sample.int(length(v), floor(sizes[i]*prop))
  bench[i] = summary(microbenchmark(
    {
      inds = sample.int(length(v), floor(sizes[i]*prop))
      v[inds]
    }, times=5, unit="s"))$mean
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
      inds = sample.int(length(v), floor(sizes[i]*prop))
      values = as.double(sample(1:10, length(inds)))
      v[inds] = values
    }, times=5, unit="s"))$median
  
}

df = cbind(df,
           data.frame(object_size=object_size, seconds=bench, memory="xmem",
                      order="random", operation="write"))
write.csv(df, "xmem_bench.csv")
