  ####  Input: token file (one review per line; tokens are delimited by white space) 
  ####         label file (one label per line)
  ####  
  ####  To display help on Step 1: enter "../bin/prepText gen_vocab"
  ####                     Step 2:       "../bin/prepText gen_b_feat"

  inpdir=../rcv1_data  # <= change this to where your RCV1 token/label files are

  #---  Step 1. Generate vocabulary
  echo Generaing uni-, bi-, and tri-gram vocabulary from training data ... 

  options="LowerCase UTF8"

  for nn in 1 2 3; do
    vocab_fn=data/rcv1-1m_trn-${nn}gram.vocab  
    ../bin/prepText gen_vocab input_fn=${inpdir}/rcv1-1m-train.txt.tok vocab_fn=$vocab_fn \
                              $options WriteCount n=$nn \
                              stopword_fn=data/rcv1_stopword.txt RemoveNumbers
  done 

  #---  Step 2.  Generate bag-of-ngram files files ...
  echo 
  echo Generating bag-of-ngram files ... 
  for nn in 1 2 3; do
    if [ "$nn" = "1" ]; then
      voc_fn=data/rcv1-1m_trn-1gram.vocab
    elif [ "$nn" = "2" ]; then
      voc_fn=data/rcv1-1m_trn-12gram.vocab
      cat data/rcv1-1m_trn-1gram.vocab > $voc_fn
      cat data/rcv1-1m_trn-2gram.vocab >>$voc_fn
    elif [ "$nn" = "3" ]; then
      voc_fn=data/rcv1-1m_trn-123gram.vocab
      cat data/rcv1-1m_trn-1gram.vocab > $voc_fn
      cat data/rcv1-1m_trn-2gram.vocab >>$voc_fn
      cat data/rcv1-1m_trn-3gram.vocab >>$voc_fn
    else
      echo "what?!"
      exit
    fi
    for set in train test; do
      outnm=data/rcv1-1m_${set}-bow${nn}
      ../bin/prepText gen_b_feat \
         vocab_fn=$voc_fn \
         input_fn=${inpdir}/rcv1-1m-${set} \
         output_fn_stem=$outnm \
         $options text_fn_ext=.txt.tok label_fn_ext=.lvl2 \
         label_dic_fn=data/rcv1-lvl2.catdic \
         LogCount Unit

      #---  To convert the bag-of-ngram feature file and the target file to the svmLight format ... 
#      for ((cat=0; cat<=54; ++cat)); do
#        echo Generating the one-vs-all file in the svmLight format ... cat${cat}
#        perl conv.pl $outnm $cat > ${outnm}.cat${cat}.xy
#      done
    done
  done
