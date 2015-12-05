  #####
  #####  Elec: parsup-tv.  tv-embedding learning with partially-supervised target. 
  #####
  #####  Step 1. tv-embedding learning from unlabeled data using partially-supervised target. 
  #####    Step 1.1. Generate input files for applying a model trained with labeled data to unlabeled data. 
  #####    Step 1.2. Apply a supervised model to unlabeled data to obtain embedded regions (i.e., output of region embedding). 
  #####    Step 1.3. Generate input files for tv-embedding training with partially-supervised target. 
  #####              Target: the embedded regions (obtained in Step 1.2) derived from the neighboring regions.  
  #####    Step 1.4. Training. 
  #####  Step 2. Supervised training using tv-embedding to produce additional input. 
  #####    Step 2.1. Generate input files. 
  #####    Step 2.2. Training. 
  #####
  #####  NOTE1: To run this script, download unlab_data.tar.gz and decompress it at test/ . 
  #####
  #####  NOTE2: For your convenience, Step 0 below downloads the supervised model.  
  #####         The model file is endian sensitive and was generated with Little Endian  
  #####         (Intel convention).  If it is not usable in your system, you need to 
  #####         generate the file in your system by modifying the provided script; see 
  #####         the comments on wget below.  
  #####

  gpu=-1  # <= change this to, e.g., "gpu=0" to use a specific GPU. 
  dim=100 # <= change this to change dimensionality of tv-embedding. 
  udir=unlab_data # <= change this to point to unlab-data.tar.gz 

  prep=../bin/prepText
  cnet=../bin/conText

  options="LowerCase UTF8"
  txt_ext=.txt.tok

  z=3 # to avoid name conflict with other scripts

  #***  Step 0.  prepare a supervised model used in Step 1.2.  
  f_pch_sz=3; f_pch_step=1; f_padding=$((f_pch_sz-1))
  mod_fn=for-parsup-elec-p${f_pch_sz}.supmod.ite100   # input: model trained by train_elec_seq.sh 
  if [ ! -e $mod_fn ]; then
    wget riejohnson.com/software/${mod_fn}.gz # Download the model, or do training as in train_elec_seq.sh, save the model and rename it. 
    gzip -d -f ${mod_fn}.gz                      
  fi

  #***  Step 1. tv-embedding learning from unlabeled data using partially-supervised target. 
  #---  Step 1.1. Generate input files for applying a supervised model to unlabeled data. 

  uns_lst=data/elec${z}-uns.lst
  echo ${udir}/elec-25k-unlab00  >  $uns_lst  # unlab_data.tar.gz
  echo ${udir}/elec-25k-unlab01 >> $uns_lst   # unlab_data.tar.gz

  #---  vocabulary for X (regions)
  xszk=30
  xvoc=data/elec${z}-25k-trn.vocab
  $prep gen_vocab input_fn=data/elec-25k-train.txt.tok vocab_fn=$xvoc $options WriteCount \
                  max_vocab_size=${xszk}000

  opt2="AllowZeroRegion RegionOnly"
  rnm=data/elec${z}-uns-p${f_pch_sz}
  #---  generate a region file of unlabeled data.
  $prep gen_regions region_fn_stem=$rnm input_fn=$uns_lst vocab_fn=$xvoc \
         $options $opt2 text_fn_ext=$txt_ext \
         patch_size=$f_pch_sz patch_stride=$f_pch_step padding=$f_padding

  
  #---  Step 1.2. Apply a supervised model to unlabeled data to obtain embedded regions. 
  top_num=10 # retain the 10 largest values only
  emb_fn=${rnm}.emb.smats
  $cnet $gpu write_embedded \
       test_mini_batch_size=100 \
       datatype=sparse tstname=$rnm  x_ext=.xsmatvar \
       model_fn=$mod_fn num_top=$top_num embed_fn=$emb_fn


  #---  Step 1.3. Generate input files for tv-embedding training with partially-supervised target.  
  pch_sz=5; pch_step=1; padding=$((pch_sz-1))
  dist=$pch_sz
  nm=elec-parsup-p${f_pch_sz}p${pch_sz}
  $prep gen_regions_parsup x_type=Bow input_fn=$uns_lst text_fn_ext=$txt_ext \
                   scale_y=1 \
                   x_vocab_fn=$xvoc region_fn_stem=data/$nm $options \
                   patch_size=$pch_sz patch_stride=$pch_step padding=$padding \
                   f_patch_size=$f_pch_sz f_patch_stride=$f_pch_step f_padding=$f_padding \
                   dist=$dist num_top=$top_num embed_fn=$emb_fn


  #---  Step 1.4. tv-embedding training. 
  gpumem=${gpu}:1 # pre-allocate 1GB GPU memory. 
  ite1=8; ite=10
  lay0_fn=output/${nm}.dim${dim}
  logfn=log_output/elec-parsup-dim${dim}.log
  echo 
  echo tv-embedding learning with unsupervised target.  
  echo This takes a while.  See $logfn for progress and see param/unsup.param for the rest of the parameters.
  $cnet $gpumem cnn trnname=$nm data_dir=data nodes=$dim 0resnorm_width=$dim save_lay0_fn=$lay0_fn \
        step_size_decay_at=$ite1 num_iterations=$ite LessVerbose \
        @param/unsup.param > $logfn


  #***  Step 2. Supervised training using tv-embedding to produce additional input. 
  #---  Step 2.1. Generate input files. 
  for set in 25k-train test; do 
    opt=AllowZeroRegion
    #---  dataset#0: for the main layer (seq)
    rnm=data/elec${z}-${set}-p${pch_sz}seq
    $prep gen_regions $opt \
      region_fn_stem=$rnm input_fn=data/elec-${set} vocab_fn=$xvoc \
      $options text_fn_ext=$txt_ext label_fn_ext=.cat \
      label_dic_fn=data/elec_cat.dic \
      patch_size=$pch_sz patch_stride=1 padding=$((pch_sz-1))

    #---  dataset#1: for the side layer (bow)
    rnm=data/elec${z}-${set}-p${pch_sz}bow
    $prep gen_regions $opt Bow \
      region_fn_stem=$rnm input_fn=data/elec-${set} vocab_fn=$xvoc \
      $options text_fn_ext=$txt_ext RegionOnly \
      patch_size=$pch_sz patch_stride=1 padding=$((pch_sz-1))
  done

  #---  Step 2.2. Training 
  gpumem=${gpu}:4 # pre-allocate 4GB GPU memory
  logfn=log_output/elec-parsup-sup-dim${dim}.log
  perffn=perf/elec-parsup-sup-dim${dim}.csv
  s_fn=${lay0_fn}.ite${ite}.layer0
  echo 
  echo Supervised training using tv-embedding to produce additional input.   
  echo This takes a while.  See $logfn and $perffn for progress and see param/semisup.param for the rest of the parameters.
  $cnet $gpumem cnn 0side0_fn=$s_fn \
        trnname=elec${z}-25k-train-p${pch_sz} tstname=elec${z}-test-p${pch_sz} data_ext0=seq data_ext1=bow \
        evaluation_fn=$perffn test_interval=25 LessVerbose \
        reg_L2=1e-4 step_size=0.1 \
        @param/semisup.param > $logfn


