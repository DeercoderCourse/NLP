Install Guide for CUDA&Caffe&conText
====

added by Chang, I recalled the main steps for configuring CUDA and some pre

1. From this post, first install many packages.
http://caffe.berkeleyvision.org/install_apt.html

sudo apt-get install libprotobuf-dev libleveldb-dev libsnappy-dev libopencv-dev libhdf5-serial-dev protobuf-compiler
sudo apt-get install --no-install-recommends libboost-all-dev

sudo apt-get install libgflags-dev libgoogle-glog-dev liblmdb-dev

Then We will have to install cuda, otherwise when building there is blas error:

from this post: https://developer.nvidia.com/cuda-downloads


2. Get the intall packages from CUDA website(nvidia)

http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1404/x86_64/cuda-repo-ubuntu1404_7.5-18_amd64.deb

Installation Instructions:
 
`sudo dpkg -i cuda-repo-ubuntu1404_7.5-18_amd64.deb`
`sudo apt-get update`
`sudo apt-get install cuda`

OpenCV is also essential, otherwise there will be a mistake for building at last stage. using this github repo to install it:

https://github.com/jayrambhia/Install-OpenCV/tree/master/Ubuntu

### Important HERE! If missing then the som compilation is failed of CUDA!!
And there still some error about OpenCV, add some linked libs at Makefile, as in this post does: https://github.com/BVLC/caffe/issues/2288

old: LIBRARIES += glog gflags protobuf boost_system m hdf5_hl hdf5
new(from thor):  LIBRARIES += glog gflags protobuf leveldb snappy \  
           lmdb boost_system hdf5_hl hdf5 m \ 
           opencv_core opencv_highgui opencv_imgproc 

### Addition
That won't fix the build problem. In fact, using cmake is better without worrying about the error

Use the following command to build all the caffe to avoid the error.

mkdir build && cd build
cmake ..
make all -j8
make runtest

# CUDNN

This is part of accelerated CNN in CUDA, it provides faster training speed for CNN, and we need to register developer account and download link
for this software.

NOTE: conText uses CNN a lot, it's needed in the makefile of some flags, so unable to install cuDNN will result in compile error.

In order to speed up training, using cuDNN, we need to install and open the flag in Make.config:

cuDNN Caffe: for fastest operation Caffe is accelerated by drop-in integration of NVIDIA cuDNN. To speed up your Caffe models, install cuDNN then uncomment the USE_CUDNN := 1 flag in Makefile.config when installing Caffe. Acceleration is automatic. For now cuDNN v1 is integrated but see PR #1731 for v2.

For installing, its like this post: https://groups.google.com/forum/#!topic/caffe-users/nlnMFI0Mh7M

tar -xzvf cudnn-6.5-linux-R1.tgz
cd cudnn-6.5-linux-R1
sudo cp lib* /usr/local/cuda/lib64/
sudo cp cudnn.h /usr/local/cuda/include/
After these steps, caffe must be rebuilt. That's to use the cmake commander again.

After that, just build and it works well!


## For other errors

I cannot recall all the errors when configuring the envrironment, but if encountered some error, I just googled and find some possible solution. The installation of
CUDA for GPU usage is a little complex but is essential for the training/testing of the conText.


