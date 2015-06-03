library(ggplot2)


mptf2ln <- read.table("tuning_mptf2ln.txt", header=TRUE)

str(mptf2ln)
summary(mptf2ln)

# best MAP value
mptf2ln[which.max(mptf2ln$MAP),]
# s   mu alpha lambda      MAP
# 0.4 2000   0.2    0.6 0.663213


ggplot(mptf2ln, aes(x=alpha, y=MAP, group=interaction(s,mu), color=interaction(s,mu))) + 
  geom_line() + geom_point() + facet_grid(. ~ lambda)

ggplot(mptf2ln, aes(x=alpha, y=MAP, group=interaction(s,mu), color=as.factor(mu))) + 
  geom_line() + geom_point() + facet_grid(. ~ lambda)




data <- subset(mptf2ln, lambda>0.4 & mu>100 & mu < 10000)

ggplot(data, aes(x=alpha, y=MAP, group=interaction(s,mu), color=interaction(s,mu))) + 
  geom_line() + geom_point() + facet_grid(. ~ lambda)


data1 <- subset(data, lambda==.6)

ggplot(data1, aes(x=alpha, y=MAP, group=interaction(s,mu), color=as.factor(s))) + 
  geom_line() + geom_point()
ggplot(data1, aes(x=alpha, y=MAP, group=interaction(s,mu), color=as.factor(mu))) + 
  geom_line() + geom_point()
max(data1$MAP)

# alpha = 0.3
# lambda = 0.6
# s = 0.4
# mu = 2000

data1 <- subset(data, lambda==.7)

ggplot(data1, aes(x=alpha, y=MAP, group=interaction(s,mu), color=as.factor(s))) + 
  geom_line() + geom_point()
ggplot(data1, aes(x=alpha, y=MAP, group=interaction(s,mu), color=as.factor(mu))) + 
  geom_line() + geom_point()
max(data1$MAP)



############################