#!/usr/bin/env python
"""
Author: Chang Liu(chang_liu@student.uml.edu)

Description: N-gram implementation, for training and testing.
"""
import collections
import math

DEBUG = True
TEST = False

def dbgprint(s):
    if DEBUG:
        print s

# the input is the text that contains all the tokens(using List)
def unigrams(text):
    uni = []
    for token in text:
        uni.append([token])
    return uni

# the input is the text that contains all the tokens(using List)
def bigrams(text):
    bi = []
    token_addr = 0
    for token in text[:len(text) - 1]:
        bi.append([token, text[token_addr + 1]])
        token_addr += 1
    return bi

# the input is the text that contains all the tokens(using List)
def trigrams(text):
    tri = []
    token_addr = 0
    for token in text[:len(text) - 2]:
        tri.append([token, text[token_addr + 1], text[token_addr + 2]])
        token_addr += 1
    return tri

# the input is the text that contains all the tokens(using List)
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
        list_new = [word.lower() for word in lists]
        #print lists
        #print file
        tokens = tokens + list_new
    return tokens

## Test the result for x-gram algorithms
def test_gram():
    result = load_text("sample-training-data.txt")
    uniresult = unigrams(result)
    biresult = bigrams(result)
    triresult = trigrams(result)
    fourresult = ngram(result, 4)

    # TEST n-gram result(output ALL grams, but not dicts, as repeat items)
    #dbgprint(result)
    #dbgprint(uniresult)
    #dbgprint(biresult)
    #dbgprint(triresult)
    #dbgprint(fourresult)

    get_grams_dict(uniresult)
    get_grams_dict(biresult)
    get_grams_dict(triresult)

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
            #dbgprint("index = %s"%index)
            seen_account[index] += 1

    # for DEBUG:
    #print "raw ", listitems, len(listitems)
    #print "seen ", seen, len(seen)
    #print "uniq ", uniq, len(uniq)
    #print "seen_account ", seen_account, len(seen_account)
    #print seen, seen_account

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


# for unigram model, parse is different from other models
def load_model(train_model_name):
    grams = []
    counts = []

    handler = open(train_model_name, "r+")

    for item in handler:
        items = item.split('\t')
        #print items
        # eliminate the " " of the string after splitting
        #print "*** items ***", items[0], items[1]
        grams.append([items[0][2:-2]])
        # account ends with "\r\n" when writing
        counts.append(int(items[1][:-2]))

    #print "load_model: gram and counts = ", grams, counts
    return grams, counts

# 2-gram model is different from unigram since it has multiple words
# here it's quite complex to parse the model, as even though we treat it
# as list[], when parsing it regards as String, so I need to parse each word
# and then construct a new Insert list to append
def load_2gram_model(train_model_name):
    grams = []
    counts = []
    insert = []

    handler = open(train_model_name, "r+")

    for item in handler:
        items = item.split('\t')
        # item[0] looks like list[], but it's string after parsing, reform it
        #print "#### items[0]", items[0]
        #print "#### items[1]", items[1][:-2]
        #print "#### splits", items[0][2:-2].split("'")
        tmp = items[0][2:-2].split("'")
        insert.append(tmp[0])
        insert.append(tmp[-1])

        grams.append(insert)
        counts.append(int(items[1][:-2]))
        insert = []

    #print "***********", grams
    return grams, counts


## Get unigram probability, Input is the model, test data, and delta
## For OOV(Out of Vacabulary), I set it as the 1.0/sum_count
def get_unigram_prob(train_model_name, test_file_name, delta ):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_model(train_model_name)

    probability = 0
    total_count = sum(model_count)
    #print "total_count", total_count

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        #print "linesets: ", line_set
        line_set_new = [word.lower() for word in line_set]
        gram = unigrams(line_set_new)
        #print "line = ", line
        #print "gram = ", gram

        line_probability = 1
        for uniset in gram:
            #print uniset
            # item in the model, then get its count and probability
            if uniset in model_gram:
               index = model_gram.index(uniset)
               this_item_total_count = model_count[index]
               #print "this_item_total_count", this_item_total_count
               this_probability = (this_item_total_count + delta)/(total_count + delta)
               #print this_probability
            # not in this list(OOV, out of vacabulary), using add-smothing it's 1
            else:
                this_count = 0
                this_probability = 1.0/total_count
            line_probability *= this_probability
        #print "line_probability = ", line_probability

        probability += math.log(line_probability, 10)

    #print 'unigram probability = ', probability
    return probability

## Get bigram probability, Input is the model, test data and delta for smoothing
def get_bigram_prob(train_model_name, test_file_name, delta ):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_2gram_model(train_model_name)

    probability = 0
    total_count = sum(model_count)
    print "total_count", total_count
    print "model_gram", model_gram

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        line_set_new = [word.lower() for word in line_set]
        print "linesets: ", line_set_new
        gram = bigrams(line_set_new)
        print "line = ", line

        line_probability = 1
        for biset in gram:
            print biset
            # item in the model, then get its count and probability
            if biset in model_gram:
               index = model_gram.index(biset)
               exact_count = model_count[index]
               print "###index=%d, count=%d"%(index, exact_count)

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
                this_probability = 1.0/total_count
            line_probability *= this_probability
        print "line_probability = ", line_probability

        probability += math.log(line_probability, 10)

    print 'bigram probability = ', probability
    return probability


## Get trigram probability, Input is the whole text
def get_trigram_prob(train_model_name, test_file_name, delta ):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_model(train_model_name)

    probability = 0
    total_count = sum(model_count)
    print "total_count", total_count

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        line_set_new = [word.lower() for word in line_set]
        print "linesets: ", line_set_new
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

## for debugging, just test some function
def debug():
    if TEST == True:
        test_gram()
        test_save_models()

def parse_parameter():

    return 1

## Main function, entry for this file
if __name__ == "__main__":
    parse_parameter()

    # this is to use the training dataset to save models as output_n_gram.model
    if TEST == True:
        test_gram()
        test_save_models()

    # now use the model to do the testing
    uni_likelihood = get_unigram_prob("output_1_gram.model", "sample-test-data.txt", 0.0)
    bi_likelihood = get_bigram_prob("output_2_gram.model", "sample-test-data.txt", 0.0)
    #tri_likelihood = get_trigram_prob("output_3_gram.model", "sample-test-data.txt", 0.0001)

    print "uni_likelihood = ", uni_likelihood
    print "bi_likelihood =", bi_likelihood
    #print "tri_likelihoo =", tri_likelihood
