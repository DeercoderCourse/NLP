README
==

##Overview

All these code must be run on the machine that are equipped with GPU. Here I have configured that makefile so that it can be run directly on the server, but
first you should make sure that the server machine should have install GPU with the CUDA drivers.

##Installation of CUDA
The installation of CUDA are quite complex for different types of the GPU hardware, here I listed some problems and installation guide, as the document preInstall.md
shows. I can recall most of the steps, but since it is a little complex to configure in the server, I can only make sure that most of the errors I encountered have
been recorded, or the necessary packages are listed.


##Dataset
for dataset, please check: http://riejohnson.com/software/elec2.tar.gz to download the elec, and http://riejohnson.com/software/unlab_data.tar.gz for unlab_data.tar.gz

We don't such a large space to upload, so it's not here though, please go there for downloading.


##List

Here are some lists of files that can give you a good understanding of how it works:

*REAMDE.original*: the original README that can catch a glimpse of how this open source project work(conText)

*README_out.md*: the README for our project and description

*preInstall.md*: the necessary file that describe how to configure the CUDA on GPU, this is essential and necessary for running the project

*conText_ug.pdf*: the original API document describing how to use the binary program to finish training, testing and tuning parameters.


##Test

Most of the our results are in the test/ folder that contains the change of the code, CNN tuned parameters, and the configurations.

The result is stored under test/perf, the log is stored under test/log_output, our tuned parameter is stored under test/param.

Some other work contains reading of the CUDA and its API document, modifying the interface for compile.

The most important thing is to learn how to use this framework, actually here after configurating the CUDA and use make to build the whole project,
the next step is try to understand how this open source project can work to finish some of our tasks:

especially about:

1) how to train a CNN network.

2) how to generate the bag-of-words(BOW) features

3) how to combine the seq-CNN, bow-CNN in this framework to finish training and testing(They extract some information to speed up the testing when training)

ALL the above works are in the script under test/ folder, which is something like `train_imdb_seq.sh`, in these shell scripts, I called some commanders that
will execute the binary program under bin/ of the source, so that they can finish the tokenization, generating of NB weight, using NB weights to generate
bag-of-words features.

It takes some time to understand each step, but luckily there are some examples under the sample/ folder. And also there is a very detailed document 
conText_ug.pdf describing how to use these API, I find this conText_ug.pdf is very useful before I get hands on the project, some playground should be used
before we design our own structure of the CNN and adjust the parameter of the network.


##Tuning parameter

There are two kinds of parameters that are closely in relation with CNN network definition, the first one is to use `./conText GPU_id cnn nodes=1000 resnorm_width=1000 data_dir=data trnname=imdb_train- tstname=imdb_test- data_ext0=p2 data_ex1=p3 reg_L2=0 top_reg_L2=1e-3 step_size=0.25 top_dropout=0.5 LessVerbose test_interval=25 evluation_fn=pref_file @param/seq2.param > ${log_fn}`

In this example, we could see that there are so many parameters can be used by the binary program of `conText`. We use it for training and testing with cnn, and then there are some remaining parameters, `nodes` refer to neuron numbers, `resnorm_width` refers to the node number, `data_dir` refer to the directory, and for the `data_ext` refers to the intermediate files ending with *.p2, *.p3, and the `reg_L2` means the L2 regularization parameter(like 1e-4, 1e-3, 1e-5, tried to find the most fit in our dataset and experiment), step_size for SGD that can fast or slow down the convergence.

The second parameter is defined under the file called `xxx.param`, if we want to change the network definition like adding new convolutional layer, we should change this file, and try to adjust some of the parameters as the above shows, this is a trial and error and then adjust with some experience practice.

##Some Modification
For some details about the work, please check the *.patch, *.param and *.sh for changes, some of the work is also stored in the following folders:

*) sample/my_changes, like `train_imdb_seq2_bown2_4gram.sh` shows, I add a new layer, and I also used the additional 4gram
*) params/backup_4layer, like `seq2_bown2_my.param` shows
*) test/backup_4layer, like `train_imdb_seq2_bown2.sh` shows, here I add another bown layer, that's why it's bown2 named

Some of the work maybe very little in the `*.sh`, so I add some comments or use a different filename with `new_xxx`.


##How to run

1) use the `preInstall.md` to make sure CUDA is installed
2) use my modified makefile to build and generate the executable program
3) run the `./sample.sh` under the sample/ folder to see if the value is correct or if any error occurs, if not there is something wrong with the project and need modifying the code or makefile or environment
4) try to run some script under the test/ folder, their should be some useful information or output.


##Misc

Some other notes about my learning of this framework is under sample/NOTE1.md, sample/xxx.md, sample/NOTE1.param and so on 
