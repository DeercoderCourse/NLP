#!/usr/bin/env python
"""
author: Chang Liu (chang_liu@student.uml.edu)
This code is used for calculating the probability with and without smoothing

NOTE: Currently this program only outputs the counter and base count, we need
to form the fractional value by hand, but it's easy to write program to do it
automatically, due to limited time I just omit this part, but actually it should
be easy, I will improve it later.
"""
import nltk
from nltk import word_tokenize
from nltk.util import ngrams

# except the beginning and ending of sentence tags
pre_str = {"We":"seek", "seek":"a", "a":"solution", "solution":"that",
            "that":"could", "could":"be", "be":"accepted", "accepted":"by",
            "by":"both", "both":"sides", "sides":"."}
pre_stats = ["We", "seek", "a", "solution", "that", "could", "be", "accepted", "by", "both", "sides", "."]
pre_items = pre_str.items()

text = open("test.txt")
raw = text.read()
raw = raw.decode("utf-8")

print raw

token = nltk.word_tokenize(raw)
bigrams = ngrams(token, 2)

f = open("output.log", "w+")

counter = [0] * len(pre_str)
print counter

statcounter = [0] * len(pre_stats)
print statcounter

for item in  list(bigrams):

    stat_count = 0
    for stat in pre_stats:
        if item[0] == stat:
           statcounter[stat_count] += 1
        stat_count += 1

    count = 0
    for it in pre_items:
        if it[0] == item[0] and it[1] == item[1]:
            print item
            counter[count] = counter[count] + 1
        count = count + 1
    f.write(str(item) + "\n")

print "bigram counters:"
print pre_str, counter
print "statistic counter for all characters:"
print pre_stats, statcounter

f.close
