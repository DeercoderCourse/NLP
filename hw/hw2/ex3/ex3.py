#!/usr/bin/env python
"""
Author: Chang Liu(chang_liu@student.uml.edu)

Description: N-gram OOV implementation, for training and testing.
"""
import collections
import random
import math
import ast
import sys
import getopt

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
        if token != '.' or token != 'UNKNOW_DOT':
            bi.append([token, text[token_addr + 1]])
        token_addr += 1
    return bi

# the input is the text that contains all the tokens(using List)
def trigrams(text):
    tri = []
    token_addr = 0
    for token in text[:len(text) - 2]:
        if token != '.' and  text[token_addr + 1] != '.':
            tri.append([token, text[token_addr + 1], text[token_addr + 2]])
        elif token != 'UNKNOW_DOT' and  text[token_addr + 1] != 'UNKNOW_DOT':
            tri.append([token, text[token_addr + 1], text[token_addr + 2]])
        token_addr += 1
    return tri

# the input is the text that contains all the tokens(using List)
def ngram(text, count_n):
    model = []
    count = 0
    flag = False

    #print "#######", text
    for token in text[:len(text) - count_n + 1]:
        for word in text[count:count + count_n - 1]:
            if word == '.' or word == 'UNKNOW_DOT':
                flag = True
        # don't contain '.' in first n-1 elements(but can be the last)
        if flag == False:
            model.append(text[count:count + count_n])
        flag = False  # reset the flag otherwise it will only contain 1st sentence
        count = count + 1
    return model

# Input: file name that contains tokenized, sentence-breaked file
# output: all tokens stored in List structure
def load_text(file):
    tokens = []
    file_handler = open(file, "r+")
    for file in file_handler:
        lists = file[:-2].split(' ')  # eliminate the \r\n at the end
        list_new = [word.lower() for word in lists]
        # print lists
        # print file
        tokens = tokens + list_new
    return tokens


# # calculate the unique grams and its counts
# # This is during the training, used to generate the dictionary
# # later we will use this to estimate the probability
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
            # dbgprint("index = %s"%index)
            seen_account[index] += 1

    # for DEBUG:
    # print "raw ", listitems, len(listitems)
    # print "seen ", seen, len(seen)
    # print "uniq ", uniq, len(uniq)
    # print "seen_account ", seen_account, len(seen_account)
    # print seen, seen_account

    return seen, seen_account

# # Save the trained model into a file
def save_training_model(n, train_file_name, model_name):
    text = load_text(train_file_name)
    model = open(model_name, "w+")

    if n == 1:
        grams = unigrams(text)
        unigram, count = get_grams_dict(grams)
        i = 0
        while i < len(unigram):
            model.write("%s\t%s\r\n" % (unigram[i], count[i]))
            i += 1
    # for other grams, need to append <\ at the beginning of sentences
    elif n == 2:
        new_padding_gram = ['<\\']
        for g in text:
            new_padding_gram.append(g)
            if g == '.':
                new_padding_gram.append('<\\')
        #print "!!!!!text, new_padding", text, new_padding_gram
        grams = bigrams(new_padding_gram)
        bigram, count = get_grams_dict(grams)
        i = 0
        while i < len(bigram):
            model.write("%s\t%s\r\n" % (bigram[i], count[i]))
            i += 1
    elif n == 3:
        new_padding_gram = ['<\\']
        for g in text:
            new_padding_gram.append(g)
            if g == '.':
                new_padding_gram.append('<\\')
        #print "!!!!!text, new_padding", text, new_padding_gram
        grams = trigrams(new_padding_gram)
        trigram, count = get_grams_dict(grams)
        i = 0
        while i < len(trigram):
            model.write("%s\t%s\r\n" % (trigram[i], count[i]))
            i += 1
    else:
        new_padding_gram = ['<\\']
        for g in text:
            new_padding_gram.append(g)
            if g == '.':
                new_padding_gram.append('<\\')
        #print "!!!!!text, new_padding", text, new_padding_gram
        grams = ngram(new_padding_gram, n)
        gram, count = get_grams_dict(grams)
        i = 0
        while i < len(gram):
            model.write("%s\t%s\r\n" % (gram[i], count[i]))
            i += 1

    model.close()

# n-gram model is different from unigram since it has multiple words
# here it's quite complex to parse the model, as even though we treat it
# as list[], when parsing it regards as String, so I need to parse each word
# and then construct a new Insert list to append
def load_ngram_model(train_model_name):
    grams = []
    counts = []

    handler = open(train_model_name, "r+")

    for item in handler:
        items = item.split('\t')
        # item[0] looks like list[], but it's string after parsing, reform it
        # print "#### items[0]", items[0]
        # print "#### items[1]", items[1][:-2]
        # print "#### splits", items[0][1:-1].split("'")
        grams.append(ast.literal_eval(items[0]))
        counts.append(int(items[1][:-2]))

    #print "***********", grams
    return grams, counts


# # Get unigram probability, Input is the model, test data, and delta
# # For OOV(Out of Vacabulary), I set it as the 1.0/sum_count
def get_unigram_prob(train_model_name, test_file_name, delta):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_ngram_model(train_model_name)

    probability = 0
    total_count = sum(model_count)
    vocabulary_count = len(model_count)
    # print "total_count", total_count

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        # print "linesets: ", line_set
        line_set_new = [word.lower() for word in line_set]
        # print "line = ", line
        # print "gram = ", gram

        # convert unknow word to UNKNOW LABEL
        unknown_file = open("sample_training_data_10p.txt", "r+")
        unknown_file_tokens = []
        for line in unknown_file.readlines():
            unknown_file_tokens += line[:-2].split(' ')

        new_label_train = []
        for word in line_set_new:
            if word in unknown_file_tokens:
                if word == '.':
                    new_label_train.append("UNKNOW_DOT")
                else:
                    new_label_train.append("UNKNOW")
            else:
                new_label_train.append(word)

        #print "##!!:", new_label_train

        gram = unigrams(new_label_train)

        line_probability = 1
        for uniset in gram:
            # print uniset
            # item in the model, then get its count and probability
            if uniset in model_gram:
                index = model_gram.index(uniset)
                this_item_total_count = model_count[index]
                # print "this_item_total_count", this_item_total_count
                this_probability = (this_item_total_count + delta) / (total_count + delta * vocabulary_count)
            # print this_probability
            # not in this list(OOV, out of vacabulary), using add-smothing it's 1
            else:
                this_count = 0
                this_probability = 1.0 / vocabulary_count
            line_probability *= this_probability
        # print "line_probability = ", line_probability

        probability += math.log(line_probability, 10)

    # print 'unigram probability = ', probability
    return probability


# # for using ngram probability
def get_ngram_prob(train_model_name, test_file_name, n, delta):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_ngram_model(train_model_name)

    probability = 0
    total_count = sum(model_count)
    vocabulary_count = len(model_count)
    #print "total_count", total_count
    #print "vacabulary_count", vocabulary_count
    #print "!!!model_gram", model_gram

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        line_set_new = [word.lower() for word in line_set]


        unknown_file = open("sample_training_data_10p.txt", "r+")
        unknown_file_tokens = []
        for line in unknown_file.readlines():
            unknown_file_tokens += line[:-2].split(' ')

        new_label_train = []
        for word in line_set_new:
            if word in unknown_file_tokens:
                if word == '.':
                    new_label_train.append("UNKNOW_DOT")
                else:
                    new_label_train.append("UNKNOW")
            else:
                new_label_train.append(word)

        #print "##!!:", new_label_train

        # print "linesets: ", line_set_new
        gram = ngram(['<\\'] + new_label_train, n)  # add start padding
        # print "line = ", line

        line_probability = 1
        for nset in gram:
            #print "###n-set", nset
            # item in the model, then get its count and probability
            if nset in model_gram:
                index = model_gram.index(nset)
                exact_count = model_count[index]
                #print "###index=%d, count=%d" % (index, exact_count)

                # for conditional probability, calculate the parital count
                this_item_total_count = 0
                for i in model_gram:
                    if i[0:n - 1] == nset[0:n - 1]:
                       index = model_gram.index(i)
                       tmp_count = model_count[index]
                       this_item_total_count += tmp_count

                    #print "###this_item_total_count", this_item_total_count
                    this_probability = (exact_count + delta) / (this_item_total_count + delta * vocabulary_count)
                    #print "###this probability ", this_probability
            # not in this list, but contain the context(OOV)
            elif nset[0:n - 1] in [word[0:n - 1] for word in model_gram]:
                context_total_count = 0
                for i in model_gram:
                    if i[0:n - 1] == nset[0:n - 1]:
                        index = model_gram.index(i)
                        tmp_count = model_count[index]
                        context_total_count += tmp_count
                this_probability = delta / (delta * vocabulary_count + context_total_count)  # /context_total_count
            # OOV, all are not exist
            else:
                this_count = 0
                this_probability = 1.0 / vocabulary_count

            line_probability *= this_probability
        #print "line_probability = ", math.log(line_probability, 10)

        probability += math.log(line_probability, 10)

    #print 'n-gram probability = ', probability
    return probability


# randomly pick 10% for unknown label, training the output_unknow_gram.model
# just like the output_n_gram.model, but replace them with unknow label
def train_unknow_model(n, trainfile, modelfile):
    input = "sample_training_data_90p.txt" # 90% of original input
    oov_file = "sample_training_data_10p.txt" # unknow vacabulary


    #print "*****"

    raw_file = open(trainfile, "r+")
    new_file = open(input, "w")
    unknown_file = open(oov_file, "w")
    model = open(modelfile, "w+")

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

    new_file = open(input, "r+")
    unknown_file = open(oov_file, "r+")

    # for 90% parts, trained the ngram model, but before generate the model,
    # replace the word with UNKNOWN if in 10% file
    new_file_tokens = []
    for line in new_file.readlines():
        new_file_tokens += line[:-2].split(' ')

    unknown_file_tokens = []
    for line in unknown_file.readlines():
        unknown_file_tokens += line[:-2].split(' ')

    new_label_train = []
    for word in new_file_tokens:
        if word in unknown_file_tokens or word.lower() in unknown_file_tokens:
            if word == '.':
                new_label_train.append("UNKNOW_DOT")
            else:
                new_label_train.append("UNKNOW")
        else:
            new_label_train.append(word.lower())


    #print "####:", new_file_tokens, unknown_file_tokens
    #print "!!!!:", new_label_train

    new_file.close()
    unknown_file.close()

    ##NOW training
    if n == 1:
        grams = unigrams(new_label_train)
        unigram, count = get_grams_dict(grams)
        i = 0
        while i < len(unigram):
            model.write("%s\t%s\r\n" % (unigram[i], count[i]))
            i += 1
    else:
        new_padding_gram = ['<\\']
        for g in new_label_train:
            new_padding_gram.append(g)
            if g == 'UNKNOW_DOT':
                new_padding_gram.append('<\\')

        grams = ngram(new_padding_gram, n)
        gram, count = get_grams_dict(grams)
        i = 0
        while i < len(gram):
            model.write("%s\t%s\r\n" % (gram[i], count[i]))
            i += 1

    model.close()




def test():
    return 1

# # Test the result for x-gram algorithms
def test_gram():
    result = load_text("sample-training-data.txt")
    uniresult = unigrams(result)
    biresult = bigrams(result)
    triresult = trigrams(result)
    fourresult = ngram(result, 4)

    # TEST n-gram result(output ALL grams, but not dicts, as repeat items)
    # dbgprint(result)
    # dbgprint(uniresult)
    # dbgprint(biresult)
    # dbgprint(triresult)
    # dbgprint(fourresult)

    get_grams_dict(uniresult)
    get_grams_dict(biresult)
    get_grams_dict(triresult)

# # Test saving models
def test_save_models():
    input = "sample-training-data.txt"
    i = 1
    # this will generat output_X_gram.model, you can change to any number
    while i < 9:
        output = "output_" + str(i) + "_gram.model"
        save_training_model(i, input, output)
        i += 1

# # for debugging, just test some function
def debug():
    test_gram()
    test_save_models()  # will save the model as `output_X_gram.model`

# some old test case
def test_case():
    # now use the model to do the testing
    uni_likelihood = get_unigram_prob("output_1_gram.model", "sample-test-data.txt", 0.1)
    bi_likelihood = get_ngram_prob("output_2_gram.model", "sample-test-data.txt", 2, 0.1)
    tri_likelihood = get_ngram_prob("output_3_gram.model", "sample-test-data.txt", 3, 0.1)
    n_likelihood = get_ngram_prob("output_3_gram.model", "sample-test-data.txt", 3, 0.1)

    print "unilikelihood = ", uni_likelihood
    print "bi_likelihood = ", bi_likelihood
    print "tri_likelihood = ", tri_likelihood
    print "n_likelihood = ", n_likelihood


def parse_parameter(argv):
    trainfile = ''
    testfile = ''
    ngram = 2
    modelfile = ''
    smooth = 0.1

    try:
        opts, args = getopt.getopt(argv, "h:", ['train=', 'ngram-length=', 'model=', 'smooth=', 'test='])
    except getopt.GetoptError:
        print 'please use the format like ex3.py --train=trainfile --ngram-length=2 --model=modelfile --smooth=0.1'
        print 'or ex3.py --test=testfile --ngram-length=2 --model=modelfile'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'ex3.py --train [training-data-file] [ngram-length: int] --model [model-file]'
        elif opt == "--train":
            trainfile = arg
        elif opt == "--test":
            testfile = arg
        elif opt in "--model":
            modelfile = arg
        elif opt in "--ngram-length":
            try:
                ngram = int(arg)
            except (ValueError):
                print "wrong value for the ngram-length, must be a int value"
                sys.exit()
        elif opt in "--smooth":
            try:
                smooth = float(arg)
            except (ValueError):
                print "wrong value for the smooth value, must be a float value"
                sys.exit(0)

    return trainfile, testfile, ngram, modelfile, smooth

# # Main function, entry for this file
if __name__ == "__main__":

    # uncomment if you want to parse the parameter from commander line
    #trainfile, testfile, n, modelfile, smooth = parse_parameter(sys.argv[1:])

    if TEST == True:
        debug()
        test_case()


    # modify this to your settings
    n = 3
    trainfile = 'sample-training-data.txt'
    modelfile = 'modelfile'
    testfile = 'sample-test-data.txt'
    smooth = 0.1

    # training
    if trainfile and n and modelfile:
        print "training..."
        train_unknow_model(n, trainfile, modelfile)
        print "complete"
    if testfile:
        print "testing..."
        likelihood = 0.0
        if n == 1:
            likelihood = get_unigram_prob(modelfile, testfile, smooth)
        else:
            likelihood = get_ngram_prob(modelfile, testfile, n, smooth)

        print "ngram_likelihood with OOV(n=%d, smooth=%f) = %f" % (n, smooth, likelihood)
