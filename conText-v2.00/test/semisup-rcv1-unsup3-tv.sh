  #####
  #####  RCV1: unsup3-tv.  tv-embedding learning with unsupervised target and region vectors of bag-of-1-3grams. 
  #####
  #####  Step 1. tv-embedding learning from unlabeled data using unsupervised target and region vectors of bag-of-1-3grams. 
  #####    Step 1.1. Generate input files.  Target: neighboring regions.  
  #####    Step 1.2. training. 
  #####  Step 2. Supervised training using tv-embedding to produce additional input. 
  #####    Step 2.1. Generate input files. 
  #####    Step 2.2. Training. 

  gpu=-1  # <= change this to, e.g., "gpu=0" to use a specific GPU. 
  rcv1dir=../rcv1_data # <= change this to the directory where RCV1 labeled data is. 
  rcv1udir=../rcv1_unlab_data  # <= change this to the directory where RCV1 unlabeled data is. 
           #
           #    NOTE: We cannot provide RCV1 unlabeled data due to the copyright issue.  
           #
  dim=100 # <= change this to change dimensionality of tv-embedding. 

  prep=../bin/prepText
  cnet=../bin/conText

  options="LowerCase UTF8"
  txt_ext=.txt.tok

  z=b # to avoid name conflict with other scripts

  #***  Step 1. tv-embedding learning from unlabeled data using unsupervised target and region vectors of bag-of-1-3grams. 
  #---  Step 1.1. Generate input files.

  uns_lst=data/rcv1${z}-uns.lst
  rm $uns_lst
  for ym in 609 610 611 612 701 702 703 704 705 706; do
    echo ${rcv1udir}/rcv1-199${ym} >> $uns_lst
  done

  #---  vocabulary for Y (target)
  yszk=30
  yvoc=data/rcv1${z}-stop-uns.vocab
  stop_fn=data/rcv1_stopword.txt
  $prep gen_vocab input_fn=$uns_lst vocab_fn=$yvoc $options WriteCount text_fn_ext=$txt_ext stopword_fn=$stop_fn \
                  max_vocab_size=${yszk}000 RemoveNumbers

  #---  vocabulary for X (regions)
  fns=
  for nn in 1 2 3; do
    xvoc=data/rcv1${z}-trn-${nn}gram.vocab
    $prep gen_vocab input_fn=${rcv1dir}/rcv1-1m-train.txt.tok vocab_fn=$xvoc $options WriteCount \
                  n=$nn
    fns="$fns $xvoc"
  done
  xszk=30
  xvoc=data/rcv1${z}-trn-123gram-${xszk}k.vocab
  echo Merging and sorting 1-3grams ... 
  perl merge_dic.pl ${xszk}000 $fns > $xvoc

  #---  split text into 5 batches
  batches=5
  $prep split_text input_fn=$uns_lst ext=$txt_ext num_batches=$batches random_seed=1 output_fn_stem=data/rcv1${z}-uns

  #---  Generate region files and a target files of 5 batches. 
  pch_sz=20; pch_step=2; padding=$((pch_sz-1))
  dist=$pch_sz

  nm=rcv1-unsx3-p${pch_sz}
  for no in 1 2 3 4 5; do
    batch_id=${no}of${batches}
    inp_fn=data/rcv1${z}-uns.${batch_id}
    $prep gen_regions_unsup x_type=Bow input_fn=$inp_fn text_fn_ext=$txt_ext \
                   $opt2 x_vocab_fn=$xvoc y_vocab_fn=$yvoc region_fn_stem=data/${nm} $options \
                   patch_size=$pch_sz patch_stride=$pch_step padding=$padding dist=$dist \
                   MergeLeftRight batch_id=$batch_id
  done


  #---  Step 1.2. tv-embedding training. 
  gpumem=${gpu}:1 # pre-allocate 1GB GPU memory
  ite1=8; ite=10  # "ite1=4; ite=5" is good enough, though. 
  lay0_fn=output/${nm}.dim${dim}
  logfn=log_output/rcv1-unsx3-dim${dim}.log
  echo 
  echo tv-embedding learning with unsupervised target.  
  echo This takes a while.  See $logfn for progress and see param/unsup.param for the rest of the parameters.
  $cnet $gpumem cnn num_batches=$batches trnname=$nm data_dir=data nodes=$dim 0resnorm_width=$dim save_lay0_fn=$lay0_fn \
        step_size_decay_at=$ite1 num_iterations=$ite LessVerbose \
        @param/unsup.param > $logfn



  #***  Step 2. Supervised training using tv-embedding to produce additional input. 
  #---  Step 2.1. Generate input files.
  xvoc1=data/rcv1${z}-1m-trn-${xszk}k.vocab
  $prep gen_vocab input_fn=${rcv1dir}/rcv1-1m-train.txt.tok vocab_fn=$xvoc1 $options WriteCount max_vocab_size=${xszk}000
  for set in train test; do 
    #---  dataset#0: for the main layer (bow)
    rnm=data/rcv1${z}-1m-${set}-p${pch_sz}bow   
    $prep gen_regions Bow VariableStride WritePositions \
      region_fn_stem=$rnm input_fn=${rcv1dir}/rcv1-1m-${set} vocab_fn=$xvoc1 \
      $options text_fn_ext=$txt_ext label_fn_ext=.lvl2 \
      label_dic_fn=data/rcv1-lvl2.catdic \
      patch_size=$pch_sz patch_stride=2 padding=$((pch_sz-1))

    #---  dataset#1: for the side layer (bag-of-1-3grams)
    pos_fn=${rnm}.pos  # make regions at the same locations as above. 
    rnm=data/rcv1${z}-1m-${set}-p${pch_sz}x3bow
    $prep gen_regions Bow input_pos_fn=$pos_fn \
      region_fn_stem=$rnm input_fn=${rcv1dir}/rcv1-1m-${set} vocab_fn=$xvoc \
      $options text_fn_ext=$txt_ext RegionOnly \
      patch_size=$pch_sz
  done

  #---  Step 2.2. Training 
  gpumem=${gpu}:4
  logfn=log_output/rcv1-unsx3-sup-dim${dim}.log
  perffn=perf/rcv1-unsx3-sup-dim${dim}.csv
  s_fn=${lay0_fn}.ite${ite}.layer0
  echo 
  echo Supervised training using tv-embedding to produce additional input.   
  echo This takes a while.  See $logfn and $perffn for progress and see param/rcv1-semisup.param for the rest of the parameters.
  $cnet $gpumem cnn 0side0_fn=$s_fn datatype=sparse_multi 0side0_dsno=1 \
        trnname=rcv1${z}-1m-train-p${pch_sz} tstname=rcv1${z}-1m-test-p${pch_sz} data_ext0=bow data_ext1=x3bow \
        evaluation_fn=$perffn test_interval=25 LessVerbose \
        reg_L2=1e-4 step_size=0.5 \
        @param/rcv1-semisup.param > $logfn

