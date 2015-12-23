  ####  Input: token file (one article per line; tokens are delimited by white space) 
  ####         label file (one label per line)
  ####  The input files are not included in the package due to copyright.  

  gpu=-1  # <= change this to, e.g., "gpu=0" to use a specific GPU. 
  mem=2   # pre-allocate 2GB device memory 
  gpumem=${gpu}:${mem} 
  inpdir=../rcv1_data  # <= change this to where rcv1 data is. 

  prep_exe=../bin/prepText
  cnn_exe=../bin/conText

  #---  Step 1. Generate vocabulary
  echo Generaing vocabulary from training data ... 

  max_num=30000
  vocab_fn=data/rcv1-1m_trn-${max_num}.vocab
  options="LowerCase UTF8"
  
  $prep_exe gen_vocab input_fn=${inpdir}/rcv1-1m-train.txt.tok vocab_fn=$vocab_fn max_vocab_size=$max_num \
                            $options WriteCount RemoveNumbers stopword_fn=data/rcv1_stopword.txt


  #---  Step 2. Generate region files (data/*.xsmatvar) and target files (data/*.y) for training and testing CNN.  
  #     We generate region vectors of the convolution layer and write them to a file, instead of making them 
  #     on the fly during training/testing.   
  echo 
  echo Generating region files ... 

  pch_sz=20

  for set in train test; do 
    rnm=data/rcv1-1m-${set}-p${pch_sz}

    #---  NOTE: The parameters are the same as IMDB.  
    $prep_exe gen_regions \
      Bow-convolution VariableStride \
      region_fn_stem=$rnm input_fn=${inpdir}/rcv1-1m-${set} vocab_fn=$vocab_fn \
      $options text_fn_ext=.txt.tok label_fn_ext=.lvl2 \
      label_dic_fn=data/rcv1-lvl2.catdic \
      patch_size=$pch_sz patch_stride=2 padding=$((pch_sz-1))
  done


  #---  Step 3. Training and test using GPU
  log_fn=log_output/rcv1-1m.log
  perf_fn=perf/rcv1-1m-perf.csv
  echo 
  echo Training CNN and testing ... 
  echo This takes a while.  See $log_fn and $perf_fn for progress and see param/seq-bow.param for the rest of the parameters. 
  nodes=1000 # number of neurons (weight vectors) in the convolution layer. 
  $cnn_exe $gpumem cnn \
         data_dir=data trnname=rcv1-1m-train-p${pch_sz} tstname=rcv1-1m-test-p${pch_sz} \
         reg_L2=1e-4 step_size=0.500 \
         nodes=$nodes resnorm_width=$nodes \
         pooling_type=Avg num_pooling=10 \
         LessVerbose test_interval=25 evaluation_fn=$perf_fn \
         @param/seq-bow.param > ${log_fn}

