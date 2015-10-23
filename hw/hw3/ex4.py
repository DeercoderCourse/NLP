#!/usr/bin/env python
import re
import math

D1 = "dance little baby, dance up high, \
never mind baby, mother is nigh; \
crow and caper, caper and crow, \
there little baby, there you go;"

D2 = "up to the ceiling, down to the ground \
backwards and forwards, round and round. \
dance little baby, mother will sing, \
with the merry coral, ding, ding, ding."

pattern = re.compile("[\s|,|.|;|]")
list1 = pattern.split(D1)
list2 = pattern.split(D2)

list1_reg = []
list2_reg = []

for item in list1:
    if len(item) >= 1:
        list1_reg.append(item)

for item in list2:
        if len(item) >= 1:
            list2_reg.append(item)

## Test whether our parse is correct
print "D1.count = %d, D1 = %s " % (len(list1_reg), list1_reg)
print "D2.count = %d, D2 = %s " % (len(list2_reg), list2_reg)


## Now we need to build the dict
set1 = set(list1_reg)
set2 = set(list2_reg)
intersect = set1.intersection(set2)
union = set1.union(set2)

# test whether the unique value of list is correct, we want to eliminate
# repeated values
print "set1", set1
print "set2", set2
print "intersection", intersect
print "union", union
print "union size", len(union)

setsize1 = len(set1)
setsize2 = len(set2)

dict1 = dict()
dict2 = dict()

for item in set1:
    dict1[item] = list1_reg.count(item)

for item in set2:
    dict2[item] = list2_reg.count(item)

print "dict1", dict1
print "dict2", dict2

### Now we try to calculate the intersection with its counter
total_common = 0
for item in intersect:
    count1 = dict1[item]
    count2 = dict2[item]
    total_common += min(count1, count2)
print "total number of common words", total_common

# for dice, just need |D1| and |D2|
dice = 2 * float(total_common) /(len(list1_reg)+len(list2_reg))
# this is the common number the D1 and D2 shared
print "Dice(D1, D2)(numberic version)", dice
dice_non = float(len(intersect)) / len(union)
print "Dice", dice_non

# for non-numeric dice, just get the union set and their common


## for Jaccard, different from dice, need union of D1 and D2, minus common
jaccard = float(total_common) / (len(list1_reg)+len(list2_reg)-total_common)
print "Jaccard(D1, D2)", jaccard

## for cos, we need vector representation of the two dicts
vec1 = dict()
vec2 = dict()

for item in union:
    if item in dict1.keys():
        vec1[item] = dict1[item]
    else:
        vec1[item] = 0

for item in union:
    if item in dict2.keys():
        vec2[item] = dict2[item]
    else:
        vec2[item] = 0
# for vectors, have to build base coordinates and get represent for ALL elements
print "vec1:", vec1
print "vec2:", vec2

# now calculate the vector multiplication
total = 0
for item in union:
    total += vec1[item] * vec2[item]

# and we also need the absolute value
distance1 = 0
distance2 = 0
for item in union:
    distance1 += vec1[item] * vec1[item]
    distance2 += vec2[item] * vec2[item]

# final value for cos
cos = float(total) / math.sqrt(distance1*distance2)
# for debugging:
print "total=%d, distance1=%d, distance2=%d"%(total, distance1, distance2)
print "cos", cos


### Now at last, for Euclidean distance
total_num = 0
for item in union:
    total_num += math.pow(vec1[item]-vec2[item], 2)

total_num = math.sqrt(total_num)
print "Euclidean distance", total_num


## for Jaccard(numeric version), we need to do more
total_min = 0
total_max = 0
for item in union:
    total_min += min(vec1[item], vec2[item])
    total_max += max(vec1[item], vec2[item])
jaccard_num = float(total_min) / total_max
print "Jaccard(numeric version)", jaccard_num
