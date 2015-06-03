library(ggplot2)


mdtf2ln <- read.table("tuning_mdtf2ln.txt", header=TRUE)

str(mdtf2ln)
summary(mdtf2ln)

# best MAP value
mdtf2ln[which.max(mdtf2ln$MAP),]


ggplot(mdtf2ln, aes(x=alpha, y=MAP, group=interaction(s,mu), color=interaction(s,mu))) + 
  geom_line() + geom_point() + facet_grid(. ~ lambda)

ggplot(mdtf2ln, aes(x=alpha, y=MAP, group=interaction(s,mu), color=as.factor(mu))) + 
  geom_line() + geom_point() + facet_grid(. ~ lambda)




data <- subset(mdtf2ln, lambda>0.7)

ggplot(data, aes(x=alpha, y=MAP, group=interaction(s,mu), color=interaction(s,mu))) + 
  geom_line() + geom_point() + facet_grid(. ~ lambda)


data1 <- subset(data, lambda==.9)

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