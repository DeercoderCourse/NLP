import collections
s = "helloworld"
aaa = ["aaa", "bbb", "aaaa", "bbb", "cccc", "bbb"]
print collections.Counter(aaa).most_common(1)[0][0]
