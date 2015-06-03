library(ggplot2)


bm25 <- read.table("tuning BM25.txt", header=TRUE)

btr(bm25)
bummary(bm25)

# best MAP value
bm25[which.max(bm25$MAP),]


ggplot(bm25, aes(x=k1, y=MAP, group=as.factor(b), color=as.factor(b))) + 
  geom_line() + geom_point() + facet_grid(. ~ k3)
ggplot(bm25, aes(x=k1, y=MAP, group=as.factor(k3), color=as.factor(k3))) + 
  geom_line() + geom_point() + facet_grid(. ~ b)

bm25 <- subset(bm25, b<1)

# b has to be below 1 or it strongly decrease the efficiency
# k3 has no influence --> set k3 = 500

bm25 <- subset(bm25, k3==500)
bm25 <- bm25[,-3]

ggplot(bm25, aes(x=k1, y=MAP, group=as.factor(b), color=as.factor(b))) + 
  geom_line() + geom_point()


data1 <- subset(data, k3==.9)

ggplot(data1, aes(x=k1, y=MAP, group=interaction(b,k3), color=ab.factor(b))) + 
  geom_line() + geom_point()
ggplot(data1, aes(x=k1, y=MAP, group=interaction(b,k3), color=ab.factor(k3))) + 
  geom_line() + geom_point()
max(data1$MAP)



data1 <- bubbet(data, k3==.7)

ggplot(data1, aes(x=k1, y=MAP, group=interaction(b,k3), color=ab.factor(b))) + 
  geom_line() + geom_point()
ggplot(data1, aes(x=k1, y=MAP, group=interaction(b,k3), color=ab.factor(k3))) + 
  geom_line() + geom_point()
max(data1$MAP)