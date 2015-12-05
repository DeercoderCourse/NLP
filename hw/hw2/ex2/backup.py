"""
All these codes are backup for history, I used these code for start up but then I find that can use a generic form of ngram function, so just backup for comparing with ngram program, maybe useful for debugging

NOTE: the program may have a very big changs comparing with previous one, this rewrite version contain many aspect about the algorithms and improvement

"""
## Get bigram probability, Input is the model, test data and delta for smoothing
def get_bigram_prob(train_model_name, test_file_name, delta ):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_ngram_model(train_model_name)

    probability = 0
    total_count = sum(model_count)
    vocabulary_count = len(model_count)
    #print "total_count", total_count
    print "[[[[[[]]]]]]model_gram", model_gram

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        line_set_new = [word.lower() for word in line_set]
        #print "linesets: ", line_set_new
        gram = bigrams(['<\\'] + line_set_new) # add start padding
        #print "line = ", line

        line_probability = 1
        for biset in gram:
            print "###biset", biset
            # item in the model, then get its count and probability
            if biset in model_gram:
               index = model_gram.index(biset)
               exact_count = model_count[index]
               print "###index=%d, count=%d"%(index, exact_count)

               # for conditional probability, calculate the parital count
               this_item_total_count = 0
               for i in model_gram:
                   if i[0] == biset[0]:
                       index = model_gram.index(i)
                       tmp_count = model_count[index]
                       this_item_total_count += tmp_count

               print "###this_item_total_count", this_item_total_count
               this_probability = (exact_count + delta)/(this_item_total_count + delta * vocabulary_count)
               print "###this probability ", this_probability
            # not in this list, but contain the context(OOV)
            elif biset[0] in [word[0] for word in model_gram]:
                context_total_count = 0
                for i in model_gram:
                    if i[0] == biset[0]:
                        index = model_gram.index(i)
                        tmp_count = model_count[index]
                        context_total_count += tmp_count
                # [fix bug]: here use total_count instead of context_total_count
                this_probability = delta/(delta * vocabulary_count + total_count) #/context_total_count
            # OOV, all are not exist
            else:
                this_count = 0
                this_probability = 1.0/total_count
            line_probability *= this_probability
        print "line_probability = ", math.log(line_probability, 10)

        probability += math.log(line_probability, 10)

    print 'bigram probability = ', probability
    return probability


## Get trigram probability, Input is the whole text
def get_trigram_prob(train_model_name, test_file_name, delta ):
    total_proba = 0
    file_handler = open(test_file_name, "r+")
    model_gram, model_count = load_ngram_model(train_model_name)

    probability = 0
    total_count = sum(model_count)
    vocabulary_count = len(model_count)
    print "total_count", total_count
    print "vacabulary_count", vocabulary_count
    print "!!!model_gram", model_gram

    # get each sentence and calculate the probability
    for line in file_handler:
        line_set = line[:-1].split(' ')
        line_set_new = [word.lower() for word in line_set]
        #print "linesets: ", line_set_new
        gram = trigrams(['<\\'] + line_set_new) # add start padding
        #print "line = ", line

        line_probability = 1
        for triset in gram:
            print "###triset", triset
            # item in the model, then get its count and probability
            if triset in model_gram:
               index = model_gram.index(triset)
               exact_count = model_count[index]
               print "###index=%d, count=%d"%(index, exact_count)

               # for conditional probability, calculate the parital count
               this_item_total_count = 0
               for i in model_gram:
                   if i[0:2] == triset[0:2]:
                       index = model_gram.index(i)
                       tmp_count = model_count[index]
                       this_item_total_count += tmp_count

               print "###this_item_total_count", this_item_total_count
               this_probability = (exact_count + delta)/(this_item_total_count + delta * vocabulary_count)
               print "###this probability ", this_probability
            # not in this list, but contain the context(OOV)
            elif triset[0:2] in [word[0:2] for word in model_gram]:
                context_total_count = 0
                for i in model_gram:
                    # for(w1,w2,w3), we sum all the (w1,w2) that in dicts
                    if i[0:2] == triset[0:2]:
                        index = model_gram.index(i)
                        tmp_count = model_count[index]
                        context_total_count += tmp_count
                # [fix bug]: use total count
                this_probability = delta/(delta * vocabulary_count + total_count) #/context_total_count
            # OOV, all are not exist
            else:
                this_count = 0
                this_probability = 1.0/total_count
            line_probability *= this_probability
        print "line_probability = ", math.log(line_probability, 10)

        probability += math.log(line_probability, 10)

    print 'trigram probability = ', probability
    return probability
