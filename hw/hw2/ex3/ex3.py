#!/usr/bin/env python
"""
Author: Chang Liu(chang_liu@student.uml.edu)

Description: N-gram OOV implementation, for training and testing.
"""
import collections
import math

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
## This is during the training, used to generate the dictionary
## later we will use this to estimate the probability
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
            model.write("%s\t%s\r\n"%(unigram[i], count[i]))
            i += 1
    elif n == 2:
        grams = bigrams(text)
        bigram, count = get_grams_dict(grams)
        i = 0
        while i < len(bigram):
            model.write("%s\t%s\r\n"%(bigram[i], count[i]))
            i += 1
    elif n == 3:
        grams = trigrams(text)
        trigram, count = get_grams_dict(grams)
        i = 0
        while i < len(trigram):
            model.write("%s\t%s\r\n"%(trigram[i], count[i]))
            i += 1
    else:
        grams = ngram(text, n)
        gram, count = get_grams_dict(grams)
        i = 0
        while i < len(gram):
            model.write("%s\t%s\r\n"%(gram[i], count[i]))
            i += 1

    model.close()


def load_model(train_model_name):
    grams = []
    counts = []

    handler = open(train_model_name, "r+")

    for item in handler:
        items = item.split('\t')
        print items
        # eliminate the " " of the string after splitting
        print "*** items ***", items[0], items[1]
        grams.append([items[0][2:-2]])
        # account ends with "\r\n" when writing
        counts.append(int(items[1][:-2]))

    print "load_model: gram and counts = ", grams, counts
    return grams, counts


## Get unigram probability, Input is the model, test data, and delta
def get_unigram_prob(train_model_name, test_file_name, delta ):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_model(train_model_name)

    probability = 0
    total_count = len(model_gram)
    print "total_count", total_count

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        print "linesets: ", line_set
        gram = unigrams(line_set)
        print "line = ", line

        line_probability = 1
        for uniset in gram:
            print uniset
            # item in the model, then get its count and probability
            if uniset in model_gram:
               index = model_gram.index(uniset)
               this_item_total_count = model_count[index]
               print "this_item_total_count", this_item_total_count
               this_probability = (this_item_total_count + delta)/(total_count + delta)
               print this_probability
            # not in this list, using add-smothing it's 1
            else:
                this_count = 0
                this_probability = 1
            line_probability *= this_probability
        print "line_probability = ", line_probability

        probability += line_probability

    print 'unigram probability = ', probability
    return math.log(probability, 10)

## Get bigram probability, Input is the model, test data and delta for smoothing
def get_bigram_prob(train_model_name, test_file_name, delta ):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_model(train_model_name)

    probability = 0
    total_count = len(model_gram)
    print "total_count", total_count

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        print "linesets: ", line_set
        gram = bigrams(line_set)
        print "line = ", line

        line_probability = 1
        for biset in gram:
            print biset
            # item in the model, then get its count and probability
            if biset in model_gram:
               index = model_gram.index(biset)
               exact_count = model_count[index]

               # for conditional probability, calculate the parital count
               this_item_total_count = 0
               for i in model_gram:
                   if i[0] == biset[0]:
                       this_item_total_count += 1

               print "this_item_total_count", this_item_total_count
               this_probability = (this_item_total_count + delta)/(exact_count + delta)
               print this_probability
            # not in this list, using add-smothing it's 1
            else:
                this_count = 0
                this_probability = 1
            line_probability *= this_probability
        print "line_probability = ", line_probability

        probability += line_probability

    print 'unigram probability = ', probability
    return math.log(probability, 10)


## Get bigram probability, Input is the whole text
def get_bigram_prob(train_model_name, test_file_name, delta ):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_model(train_model_name)

    probability = 0

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        print "linesets: ", line_set
        gram = bigrams(line_set)
        print "line = ", line

        line_probability = 1
        for biset in gram:
            print biset
            # item in the model, then get its count and probability
            if biset in model_gram:
               index = model_gram.index(biset)
               exact_count = model_count[index]

               # for conditional probability, calculate the parital count
               this_item_total_count = 0
               for i in model_gram:
                   if i[0] == biset[0]:
                       this_item_total_count += 1

               print "this_item_total_count", this_item_total_count
               this_probability = (this_item_total_count + delta)/(exact_count + delta)
               print this_probability
            # not in this list, using add-smothing it's 1
            else:
                this_count = 0
                this_probability = 1
            line_probability *= this_probability
        print "line_probability = ", line_probability

        probability += line_probability

    print 'bigram probability = ', probability
    return math.log(probability, 10)

## Get trigram probability, Input is the whole text
def get_trigram_prob(train_model_name, test_file_name, delta ):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_model(train_model_name)

    probability = 0

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        print "linesets: ", line_set
        gram = trigrams(line_set)
        print "line = ", line

        line_probability = 1
        for triset in gram:
            print triset
            # item in the model, then get its count and probability
            if triset in model_gram:
               index = model_gram.index(triset)
               exact_count = model_count[index]

               # for conditional probability, calculate the parital count
               this_item_total_count = 0
               for i in model_gram:
                   if i[0:2] == triset[0:2]:
                       this_item_total_count += 1

               print "this_item_total_count", this_item_total_count
               this_probability = (this_item_total_count + delta)/(exact_count + delta)
               print this_probability
            # not in this list, using add-smothing it's 1
            else:
                this_count = 0
                this_probability = 1
            line_probability *= this_probability
        print "line_probability = ", line_probability

        probability += line_probability

    print 'trigram probability = ', probability
    return math.log(probability, 10)

def train():
    return 1


def test():
    return 1

def parse_parameter():

    return 1

# randomly pick 10% for training the output_unknow_gram.model
# just like the output_n_gram.model
def train_unknow_model():
    file = "sample-training-data.txt"
    input = "sample_training_data_90p.txt" # 90% of original input
    oov_file = "sample_training_data_10p.txt" # unknow vacabulary

    raw_file = open(file, "r+")
    new_file = open(input, "a")
    unknown_file = open(oov_file, "a")

    count = 0
    for line in raw_file:
        if count % 10 == 0:
            unknown_file.write(line)
        else:
            new_file.write(line)
        count += 1

    raw_file.close()
    new_file.close()
    unknown_file.close()

    # for 90% parts, trained the ngram model
    i = 1
    while i < 5:
        output = "output_" + str(i) + "_gram.model"
        save_training_model(i, input, output)
        i += 1

    # for remaing parts, build the unknown dictionary
    i = 1
    while i < 5:
        output = "unknown_" + str(i) + "_gram.model"
        save_training_model(i, oov_file, output)
        i += 1

## Main function, entry for this file
if __name__ == "__main__":
    parse_parameter()

    # this is to use the training dataset to save models as output_n_gram.model
    if TEST == True:
        test_gram()
        test_save_models()
        train_unknow_model()

    # now use the model to do the testing
    uni_likelihood = get_unigram_prob("output_1_gram.model", "sample-test-data.txt", 0.0001)
    bi_likelihood = get_bigram_prob("output_2_gram.model", "sample-test-data.txt", 0.0001)
    tri_likelihood = get_trigram_prob("output_3_gram.model", "sample-test-data.txt", 0.0001)

    print "uni_likelihood = ", uni_likelihood
    print "bi_likelihood =", bi_likelihood
    print "tri_likelihoo =", tri_likelihood
