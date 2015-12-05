  #####
  #####  IMDB: unsup-tv.  tv-embedding learning with unsupervised target. 
  #####
  #####  Step 1. tv-embedding learning from unlabeled data using unsupervised target. 
  #####    Step 1.1. Generate input files.  Target: neighboring regions. 
  #####    Step 1.2. Training. 
  #####  Step 2. Supervised training using tv-embedding to produce additional input. 
  #####    Step 2.1. Generate input files. 
  #####    Step 2.2. Training. 
  #####
  #####  NOTE: To run this script, download unlab_data.tar.gz and decompress it at test/ . 
  #####

  gpu=-1  # <= change this to, e.g., "gpu=0" to use a specific GPU. 
  dim=100 # <= change this to change dimensionality of tv-embedding. 
  udir=unlab_data # <= change this to point to unlab_data.tar.gz 

  prep=../bin/prepText
  cnet=../bin/conText

  options="LowerCase UTF8"
  txt_ext=.txt.tok

  z=1 # to avoid name conflict with other scripts

  #***  Step 1. tv-embedding learning from unlabeled data using unsupervised target. 
  #---  Step 1.1. Generate input files. 

  uns_lst=data/imdb${z}-trnuns.lst # unlabeled data
  echo data/imdb-train       >  $uns_lst
  echo ${udir}/imdb-unlab >> $uns_lst  # unlab_data.tar.gz

  #---  vocabulary for Y (target)
  yszk=30
  yvoc=data/imdb${z}-minstop-uns.vocab
  stop_fn=${udir}/minstop.txt   # unlab_data.tar.gz
  $prep gen_vocab input_fn=$uns_lst vocab_fn=$yvoc $options WriteCount text_fn_ext=$txt_ext stopword_fn=$stop_fn \
                  max_vocab_size=${yszk}000

  #---  vocabulary for X (regions)
  xszk=30
  xvoc=data/imdb${z}-trn.vocab
  $prep gen_vocab input_fn=data/imdb-train.txt.tok vocab_fn=$xvoc $options WriteCount \
                  max_vocab_size=${xszk}000

  #---  Generate a region file and a target file. 
  pch_sz=5; pch_step=1; padding=$((pch_sz-1))
  dist=$pch_sz
  nm=imdb-uns-p${pch_sz}
  $prep gen_regions_unsup x_type=Bow input_fn=$uns_lst text_fn_ext=$txt_ext \
                   $opt2 x_vocab_fn=$xvoc y_vocab_fn=$yvoc region_fn_stem=data/${nm} $options \
                   patch_size=$pch_sz patch_stride=$pch_step padding=$padding dist=$dist 


  #---  Step 1.2. tv-embedding training. 
  gpumem=${gpu}:1  # pre-allocate 1GB GPU memory.
  ite1=8; ite=10
  lay0_fn=output/${nm}.dim${dim}
  logfn=log_output/imdb-uns-dim${dim}.log
  echo 
  echo tv-embedding learning with unsupervised target.  
  echo This takes a while.  See $logfn for progress and see param/unsup.param for the rest of the parameters.
  $cnet $gpumem cnn trnname=$nm data_dir=data nodes=$dim 0resnorm_width=$dim save_lay0_fn=$lay0_fn \
        step_size_decay_at=$ite1 num_iterations=$ite LessVerbose \
        @param/unsup.param > $logfn


  #***  Step 2. Supervised training using tv-embedding to produce additional input. 
  #---  Step 2.1. Generate input files.
  for set in train test; do 
    opt=AllowZeroRegion
    #---  dataset#0: for the main layer (seq)
    rnm=data/imdb${z}-${set}-p${pch_sz}seq
    $prep gen_regions $opt \
      region_fn_stem=$rnm input_fn=data/imdb-${set} vocab_fn=$xvoc \
      $options text_fn_ext=$txt_ext label_fn_ext=.cat \
      label_dic_fn=data/imdb_cat.dic \
      patch_size=$pch_sz patch_stride=1 padding=$((pch_sz-1))

    #---  dataset#1: for the side layer (bow)
    rnm=data/imdb${z}-${set}-p${pch_sz}bow
    $prep gen_regions $opt Bow \
      region_fn_stem=$rnm input_fn=data/imdb-${set} vocab_fn=$xvoc \
      $options text_fn_ext=$txt_ext RegionOnly \
      patch_size=$pch_sz patch_stride=1 padding=$((pch_sz-1))
  done

  #---  Step 2.2. Training 
  gpumem=${gpu}:4  # pre-allocate 4GB GPU memory. 
  logfn=log_output/imdb-uns-sup-dim${dim}.log
  perffn=perf/imdb-uns-sup-dim${dim}.csv
  s_fn=${lay0_fn}.ite${ite}.layer0
  echo 
  echo Supervised training using tv-embedding to produce additional input.   
  echo This takes a while.  See $logfn and $perffn for progress and see param/semisup.param for the rest of the parameters.
  $cnet $gpumem cnn 0side0_fn=$s_fn \
        trnname=imdb${z}-train-p${pch_sz} tstname=imdb${z}-test-p${pch_sz} data_ext0=seq data_ext1=bow \
        evaluation_fn=$perffn test_interval=25 LessVerbose \
        reg_L2=1e-4 step_size=0.1 \
        @param/semisup.param > $logfn
