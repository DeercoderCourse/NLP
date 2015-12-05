####  Change CUDA_PATH and -gencode options in CFLAGS1 if necessary. 

BIN_NAME1 = conText
BIN_DIR = bin
TARGET1 = $(BIN_DIR)/$(BIN_NAME1)

CUDA_PATH       = /usr/local/cuda
CUDA_INC_PATH   = $(CUDA_PATH)/include
CUDA_BIN_PATH   = $(CUDA_PATH)/bin
CUDA_LIB_PATH   = $(CUDA_PATH)/lib64
LDFLAGS1   = -L$(CUDA_LIB_PATH) -lcudart -lcublas -lcurand -lcusparse
CFLAGS1 = -Isrc/com -Isrc/data -Isrc/cnet  -D__AZ_SMAT_SINGLE__ -D__AZ_GPU__  -I$(CUDA_INC_PATH) -O2 \
                -gencode arch=compute_52,code=sm_52

CPP_FILES1= 	\
	src/com/AzDmat.cpp \
	src/com/AzParam.cpp \
	src/com/AzSmat.cpp \
	src/com/AzStrPool.cpp \
	src/com/AzTextMat.cpp \
	src/com/AzTools.cpp \
	src/com/AzUtil.cpp \
	src/cnet/AzpPatchDflt.cpp \
	src/cnet/AzpCNet3.cpp \
	src/cnet/AzpLayer.cpp \
	src/cnet/AzpMain_conText.cpp \
	src/cnet/AzpLmSgd.cpp \
	src/cnet/AzPmat.cpp \
	src/cnet/AzPmatSpa.cpp \
	src/cnet/AzPmatApp.cpp \
	src/cnet/AzPmat_gpu.cu \
	src/cnet/AzPmatSpa_gpu.cu \
	src/cnet/AzCuda_Pmat.cu \
	src/cnet/AzCuda_PmatSpa.cu \
	src/cnet/AzCuda_PmatApp.cu \
	src/cnet/AzpEv.cpp \
	src/cnet/AzpLossDflt.cpp \
	src/cnet/driv_conText.cpp \
	src/cnet/AzpCNet3_multi.cpp \
	src/cnet/AzpCNet3_multi_side.cpp \
	src/cnet/AzpLayerConn.cpp


BIN_NAME2 = prepText
TARGET2 = $(BIN_DIR)/$(BIN_NAME2)
CFLAGS2 = -Isrc/com -O2 -D__AZ_SMAT_SINGLE__

CPP_FILES2= 	\
	src/com/AzDmat.cpp \
	src/com/AzParam.cpp \
	src/com/AzSmat.cpp \
	src/com/AzStrPool.cpp \
	src/com/AzTools.cpp \
	src/com/AzTextMat.cpp \
	src/com/AzUtil.cpp \
	src/data/AzPrepText.cpp \
	src/data/AzPrepText2.cpp \
	src/data/AzTools_text.cpp \
	src/data/driv_PrepText2.cpp

all: $(TARGET1) $(TARGET2)

${TARGET1}: 
	/bin/rm -f $(TARGET1)
	$(CUDA_BIN_PATH)/nvcc $(CPP_FILES1) $(CFLAGS1) -o $(TARGET1) $(LDFLAGS1)

${TARGET2}:
	/bin/rm -f $(TARGET2)
	g++ $(CPP_FILES2) $(CFLAGS2) -o $(TARGET2)

clean: 
	/bin/rm -f $(TARGET1)
	/bin/rm -f $(TARGET2)

