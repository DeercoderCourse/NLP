  #####
  #####  RCV1: parsup-tv.  tv-embedding learning with partially-supervised target. 
  #####
  #####  Step 1. tv-embedding learning from unlabeled data using partially-supervised target. 
  #####    Step 1.1. Generate input files for applying a model trained with labeled data to unlabeled data. 
  #####    Step 1.2. Apply a supervised model to unlabeled data to obtain embedded regions (i.e., output of region embedding). 
  #####    Step 1.3. Generate input files for tv-embedding training with partially-supervised target. 
  #####              Target: embedded regions (obtained in Step 1.2) derived from the neighboring regions.  
  #####    Step 1.4. Training. 
  #####  Step 2. Supervised training using tv-embedding to produce additional input. 
  #####    Step 2.1. Generate input files. 
  #####    Step 2.2. Training. 
  #####
  #####  NOTE: For your convenience, Step 0 below downloads the supervised model.  
  #####        The model file is endian sensitive and was generated with Little Endian  
  #####        (Intel convention).  If it is not usable in your system, you need to 
  #####        generate the file in your system by modifying the provided script; see 
  #####        the comments on wget below.  
  #####

  gpu=-1  # <= change this to, e.g., "gpu=0" to use a specific GPU. 
  dim=100 # <= change this to change dimensionality of tv-embedding. 
  rcv1dir=../rcv1_data # <= change this to the directory where RCV1 labeled data is. 
  rcv1udir=../rcv1_unlab_data  # <= change this to the directory where RCV1 unlabeled data is. 
           #
           #    NOTE: We cannot provide RCV1 unlabeled data due to the copyright issue.  
           #

  prep=../bin/prepText
  cnet=../bin/conText

  options="LowerCase UTF8"
  txt_ext=.txt.tok

  z=c # to avoid name conflict with other scripts

  #***  Step 0. Prepare a supervised model
  f_pch_sz=20; f_pch_step=2; f_padding=$((f_pch_sz-1))
  mod_fn=for-parsup-rcv1-p${f_pch_sz}.supmod.ite100 # model trained by train_rcv1_bow.sh
  if [ ! -e $mod_fn ]; then
    wget riejohnson.com/software/${mod_fn}.gz # Download the model, or do training as in train_rcv1_bow.sh, save the model and rename it. 
    gzip -d -f ${mod_fn}.gz                
  fi

  #***  Step 1. tv-embedding learning from unlabeled data using partially-supervised target. 
  #---  Step 1.1. Generate input files for applying a supervised model to unlabeled data.

  uns_lst=data/rcv1${z}-uns.lst
  rm $uns_lst
  for ym in 609 610 611 612 701 702 703 704 705 706; do
    echo ${rcv1udir}/rcv1-199${ym} >> $uns_lst
  done

  #---  vocabulary for X (regions)
  xszk=30
  xvoc=data/rcv1${z}-trn.vocab
  $prep gen_vocab input_fn=${rcv1dir}/rcv1-1m-train.txt.tok vocab_fn=$xvoc $options WriteCount \
                  max_vocab_size=${xszk}000

  #---  split text into 5 batches. 
  batches=5
  $prep split_text input_fn=$uns_lst ext=$txt_ext num_batches=$batches random_seed=1 output_fn_stem=data/rcv1${z}-uns


  #---  Prepare data for training with unlabeled data
  opt2="Bow AllowZeroRegion RegionOnly"
  pch_sz=20; pch_step=1; padding=$((pch_sz-1))
  top_num=10
  dist=$pch_sz

  nm=rcv1-parsup-p${f_pch_sz}p${pch_sz}

  for no in 1 2 3 4 5; do  # for each batch
    batch_id=${no}of${batches}
    rnm=data/rcv1${z}-uns${no}-p${f_pch_sz}
    #---  generate a region file for unlabeled data. 
    $prep gen_regions region_fn_stem=$rnm input_fn=data/rcv1${z}-uns.${batch_id} vocab_fn=$xvoc \
         $options $opt2 text_fn_ext=$txt_ext \
         patch_size=$f_pch_sz patch_stride=$f_pch_step padding=$f_padding
  
    #---  Step 1.2. Apply a supervised model to unlabeled data to obtain embedded regions. 
    emb_fn=${rnm}.emb.smats
    $cnet $gpu write_embedded \
       test_mini_batch_size=100 \
       datatype=sparse tstname=$rnm  x_ext=.xsmatvar \
       model_fn=$mod_fn num_top=$top_num embed_fn=$emb_fn

    #---  Step 1.3. Generate input files for tv-embedding training with partially-supervised target.  
    $prep gen_regions_parsup x_type=Bow input_fn=data/rcv1${z}-uns.${batch_id} text_fn_ext=$txt_ext \
                   scale_y=1 MergeLeftRight \
                   x_vocab_fn=$xvoc region_fn_stem=data/$nm $options \
                   patch_size=$pch_sz patch_stride=$pch_step padding=$padding \
                   f_patch_size=$f_pch_sz f_patch_stride=$f_pch_step f_padding=$f_padding \
                   dist=$dist num_top=$top_num embed_fn=$emb_fn batch_id=$batch_id
  done

  #---  Step 1.4. tv-embedding training  
  gpumem=${gpu}:1 # pre-allocate 1GB GPU memory. 
  ite1=8; ite=10  # "ite1=4; ite=5" is good enough, though. 
  lay0_fn=output/${nm}.dim${dim}
  logfn=log_output/rcv1-parsup-dim${dim}.log
  echo 
  echo tv-embedding learning with unsupervised target.  
  echo This takes a while.  See $logfn for progress and see param/unsup.param for the rest of the parameters.
  $cnet $gpumem cnn num_batches=$batches trnname=$nm data_dir=data nodes=$dim 0resnorm_width=$dim \
        save_lay0_fn=$lay0_fn \
        step_size_decay_at=$ite1 num_iterations=$ite LessVerbose \
        @param/unsup.param > $logfn


  #***  Step 2. Supervised training using tv-embedding to produce additional input. 
  #---  Step 2.1. Generate input files.
  for set in train test; do 
    rnm=data/rcv1${z}-1m-${set}-p${pch_sz}
    $prep gen_regions Bow VariableStride \
      region_fn_stem=$rnm input_fn=${rcv1dir}/rcv1-1m-${set} vocab_fn=$xvoc \
      $options text_fn_ext=$txt_ext label_fn_ext=.lvl2 \
      label_dic_fn=data/rcv1-lvl2.catdic \
      patch_size=$pch_sz patch_stride=2 padding=$((pch_sz-1))
  done

  #---  Step 2.2. tv-embedding training 
  gpumem=${gpu}:4 # pre-allocate 4GB GPU memory
  logfn=log_output/rcv1-parsup-sup-dim${dim}.log
  perffn=perf/rcv1-parsup-sup-dim${dim}.csv
  s_fn=${lay0_fn}.ite${ite}.layer0
  echo 
  echo Supervised training using tv-embedding to produce additional input.   
  echo This takes a while.  See $logfn and $perffn for progress and see param/semisup.param for the rest of the parameters.
  $cnet $gpumem cnn 0side0_fn=$s_fn datatype=sparse \
        trnname=rcv1${z}-1m-train-p${pch_sz} tstname=rcv1${z}-1m-test-p${pch_sz} \
        evaluation_fn=$perffn test_interval=25 LessVerbose \
        reg_L2=1e-4 step_size=0.5 \
        @param/rcv1-semisup.param > $logfn

