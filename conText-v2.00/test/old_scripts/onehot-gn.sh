  #####
  #####  To use GoogleNews word2vec word vectors to produce additional input to one-hot CNNs (Fig.3) 
  #####
  #####  Download GoogleNews word vectors from the word2vec website.  
  #####  Step 1. Generate input files (region files and target file). 
  #####  Step 2. Adapt word vectors for use as weights.  
  #####  Step 3. Training/testing CNNs. 
  #####    Step 3.1. Using concat. of GN word vectors as additional input. 
  #####    Step 3.2. Using average of GN word vectors as additional input. 
  #####
  #####  This script is for IMDB.  For Elec and RCV1, change filenames where necessary 
  #####  and change training parameters as indicated below.  
  #####
  #####  ----------
  #####  To use word2vec word vectors trained on the unlabeled data of each
  #####    dataset as in Fig.4, replace gn_fn and gn_dim.  
  #####    Note: gn_dim is the dim. of word vectors, and dim. of additional input becomes 
  #####    s times larger if word vectors are concatenated and region size is s.  
  #####    Settings used in Fig.4: 
  #####      - word2vec training: Huffman and distance 5 on IMDB and Elec 
  #####                           negative sampling with 5 and distance 15 on RCV1
  #####      - region size: 5 (IMDB and Elec), 20 (RCV1) as in the other experiments
  #####      - reg_L2=1e-4 and step_size=0.05(IMDB), 0.1(Elec), and 0.25(RCV1). 
  #####

  gpu=-1           # <= change this to, e.g., "gpu=0" to use a specific GPU. 
  gn_fn=../gn_data/GoogleNews-vectors-negative300.bin # <= change this to the word2vec GoogleNews vector file 
  gn_dim=300

  prep=../bin/prepText
  cnet=../bin/conText

  txt_ext=.txt.tok

  z=6 # to avoid name conflict with other scripts

  trntst_lst=data/imdb${z}-trntst.lst
  echo data/imdb-train  >  $trntst_lst
  echo data/imdb-test   >> $trntst_lst

  #---  Step 1. Generate input files. 
  gn_opt= # Don't use "LowerCase" since GoogleNews word vectors are mixed-case. 
  gn_voc=data/imdb${z}-trntst-gn.vocab # all the words in the training set and the test set. 
  $prep gen_vocab input_fn=$trntst_lst text_fn_ext=$txt_ext vocab_fn=$gn_voc $gn_opt WriteCount

  my_opt="LowerCase"
  xszk=30
  my_voc=data/imdb${z}-trn.vocab   # vocabulary used by the base model
  $prep gen_vocab input_fn=data/imdb-train.txt.tok vocab_fn=$my_voc $my_opt WriteCount \
                  max_vocab_size=${xszk}000
  pch_sz=5
  padding=$((pch_sz-1))
  for set in train test; do 
    opt=AllowZeroRegion
    #---  seq (dataset#0, used by the base model)
    rnm=data/imdb${z}-${set}-p${pch_sz}seq
    $prep gen_regions $opt \
      region_fn_stem=$rnm input_fn=data/imdb-${set} vocab_fn=$my_voc \
      $my_opt text_fn_ext=$txt_ext label_fn_ext=.cat \
      label_dic_fn=data/imdb_cat.dic \
      patch_size=$pch_sz patch_stride=1 padding=$padding

    #---  For word vector lookup (for use as dataset#1 in Step 3.1 "one-hot+concat") 
    rnm=data/imdb${z}-${set}-p1gn
    $prep gen_regions $opt \
      region_fn_stem=$rnm input_fn=data/imdb-${set} vocab_fn=$gn_voc \
      $gn_opt text_fn_ext=$txt_ext label_fn_ext=.cat \
      label_dic_fn=data/imdb_cat.dic \
      patch_size=1 patch_stride=1 padding=0

    #---  bow (for use as dataset#1 in Step 3.2 "one-hot+avg")
    rnm=data/imdb${z}-${set}-p${pch_sz}gnbow
    $prep gen_regions $opt Bow \
      region_fn_stem=$rnm input_fn=data/imdb-${set} vocab_fn=$gn_voc \
      $gn_opt text_fn_ext=$txt_ext RegionOnly \
      patch_size=$pch_sz patch_stride=1 padding=$padding
  done


  #---  Step 2.  Adapt word vectors for use as weights. 
  wfn=data/gn-for-imdb${z}.pmat
  word_map_fn=data/imdb${z}-train-p1gn.xtext  # word-mapping file generated in Step 1 
                                              # identical with data/imdb${z}-train-p5gnbow
  $cnet $gpu adapt_word_vectors wordvec_bin_fn=$gn_fn word_map_fn=$word_map_fn weight_fn=$wfn rand_param=0.01 random_seed=1


  #---  Step 3. Training/testing CNNs. 
  #---  Step 3.1. Use concat. of word vectors as additional input to one-hot CNN. 
  gpumem=${gpu}:4
  logfn=log_output/imdb-gn-concat.log
  perffn=perf/imdb-gn-concat.csv
  echo 
  echo Training and testing one-hot-CNN with concat. of word vectors as additional input. 
  echo This takes a while.  See $logfn and $perffn for progress and see param/prev-gn.param for the rest of the parameters. 
  $cnet $gpumem cnn 0side0_weight_fn=$wfn 0side0_nodes=$gn_dim \
        0side0_PatchLater 0side0_patch_size=$pch_sz 0side0_patch_stride=1 0side0_padding=$padding \
        trnname=imdb${z}-train- tstname=imdb${z}-test- data_ext0=p${pch_sz}seq data_ext1=p1gn \
        evaluation_fn=$perffn test_interval=25 LessVerbose \
        reg_L2=1e-4 step_size=0.25 \
        @param/semisup.param > $logfn

        # Elec: step_size=0.25
        # RCV1: step_size=0.25 datatype=sparse_multi 0side0_dsno=1 @param/rcv1-semisup.param

  #---  Step 3.2. Use the average of word vectors as additional input to one-hot CNN. 
  gpumem=${gpu}:4
  logfn=log_output/imdb-gn-avg.log
  perffn=perf/imdb-gn-avg.csv
  echo 
  echo Training and testing one-hot-CNN with average of word vectors as additional input. 
  echo This takes a while.  See $logfn and $perffn for progress and see param/prev-gn.param for the rest of the parameters. 
  $cnet $gpumem cnn 0side0_weight_fn=$wfn 0side0_nodes=$gn_dim 0side0_Avg \
        trnname=imdb${z}-train- tstname=imdb${z}-test- data_ext0=p${pch_sz}seq data_ext1=p${pch_sz}gnbow \
        evaluation_fn=$perffn test_interval=25 LessVerbose \
        reg_L2=1e-4 step_size=0.1 \
        @param/semisup.param > $logfn

        # Elec: step_size=0.25
        # RCV1: step_size=0.5 datatype=sparse_multi 0side0_dsno=1 @param/rcv1-semisup.param

