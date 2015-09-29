#!/usr/bin/env python
import nltk
from nltk import word_tokenize
from nltk.util import ngrams

text = "Hi, How are you? I am fine and you"
token = nltk.word_tokenize(text)
bigrams = ngrams(token, 2)

print list(bigrams)
