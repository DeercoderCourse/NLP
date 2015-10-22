#!/usr/bin/env python
import math

number = [8.1, 1.4, 2.7, 4.2, 12.7, 6.0, 0.7, 4.0, 9.0, 7.4, 43.8]

print "log_2{4} = ", math.log(4, 2)

sum = 0
for i in number:
        sum += (i/100) * math.log(i/100, 2)

print sum
print "Entropy is ", -sum
