#!/usr/bin/env python
"""
Author: Chang Liu(chang_liu@student.uml.edu)

Description: N-gram implementation, for training and testing.
"""
import collections

DEBUG = True
TEST = True

def dbgprint(s):
    if DEBUG:
        print s

# the input is the text that contains all the tokens(using List)
def unigrams(text):
    uni = []
    for token in text:
        uni.append([token])
    return uni

def bigrams(text):
    bi = []
    token_addr = 0
    for token in text[:len(text) - 1]:
        bi.append([token, text[token_addr + 1]])
        token_addr += 1
    return bi

def trigrams(text):
    tri = []
    token_addr = 0
    for token in text[:len(text) - 2]:
        tri.append([token, text[token_addr + 1], text[token_addr + 2]])
        token_addr += 1
    return tri

def ngram(text, count_n):
    model = []
    count = 0
    for token in text[:len(text)-count_n+1]:
        model.append(text[count:count+count_n])
        count = count + 1
    return model

# Input: file name that contains tokenized, sentence-breaked file
# output: all tokens stored in List structure
def load_text(file):
    tokens = []
    file_handler = open(file, "r+")
    for file in file_handler:
        lists = file[:-2].split(' ') # eliminate the \r\n at the end
        print lists
        print file
        tokens = tokens + lists
    return tokens

## Test the result for x-gram algorithms
def test_gram():
    result = load_text("sample-training-data.txt")
    uniresult = unigrams(result)
    biresult = bigrams(result)
    triresult = trigrams(result)
    fourresult = ngram(result, 4)

    # TEST n-gram result
    dbgprint(result)
    dbgprint(uniresult)
    dbgprint(biresult)
    dbgprint(triresult)
    dbgprint(fourresult)

    get_grams_dict(uniresult)

## Test saving models
def test_save_models():
    input = "sample-training-data.txt"
    i = 1
    while i < 5:
        output = "output_" + str(i) + "_gram.model"
        save_training_model(i, input, output)
        i += 1

## calculate the unique grams and its counts
def get_grams_dict(listitems):
    seen = []
    uniq = []
    seen_account = []
    for x in listitems:
        if x not in seen:
            uniq.append(x)
            seen.append(x)
            seen_account.append(1)
        else:
            index = seen.index(x)
            dbgprint("index = %s"%index)
            seen_account[index] += 1

    print "raw ", listitems, len(listitems)
    print "seen ", seen, len(seen)
    print "uniq ", uniq, len(uniq)
    print "seen_account ", seen_account, len(seen_account)

    return seen, seen_account

## Save the trained model into a file
def save_training_model(n, train_file_name, model_name):
    text = load_text(train_file_name)
    model = open(model_name, "w+")

    if n == 1:
        grams = unigrams(text)
        unigram, count = get_grams_dict(grams)
        i = 0
        while i < len(unigram):
            model.write("%s %s\r\n"%(unigram[i], count[i]))
            i += 1
    elif n == 2:
        grams = bigrams(text)
        bigram, count = get_grams_dict(grams)
        i = 0
        while i < len(bigram):
            model.write("%s %s\r\n"%(bigram[i], count[i]))
            i += 1
    elif n == 3:
        grams = trigrams(text)
        trigram, count = get_grams_dict(grams)
        i = 0
        while i < len(trigram):
            model.write("%s %s\r\n"%(trigram[i], count[i]))
            i += 1
    else:
        grams = ngram(text, n)
        gram, count = get_grams_dict(grams)
        i = 0
        while i < len(gram):
            model.write("%s %s\r\n"%(gram[i], count[i]))
            i += 1

    model.close()

## Get unigram probability, Input is the whole text
def get_unigram_prob(test_file_name):
   total_proba = 0
   file_handler = open(test_file_name, "r+")
   for line in file_handler:
       unigrams = unigrams(line)
       print line
       grams, gramcount = get_grams_dict(unigrams)


def train():
    return 1


def test():
    return 1

def parse_parameter():

    return 1

## Main function, entry for this file
if __name__ == "__main__":
    parse_parameter()

    if TEST == True:
        test_gram()
        test_save_models()
