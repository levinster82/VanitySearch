#---------------------------------------------------------------------
# Makefile for VanitySearch
#
# Author : Jean-Luc PONS

OBJDIR = obj

OBJET = $(addprefix $(OBJDIR)/, \
        Base58.o IntGroup.o main.o Random.o Timer.o Int.o \
        IntMod.o Point.o SECP256K1.o Vanity.o GPU/GPUGenerate.o \
        hash/ripemd160.o hash/sha256.o hash/sha512.o \
        hash/ripemd160_sse.o hash/sha256_sse.o Bech32.o Wildcard.o)

ifdef gpu
OBJET += $(OBJDIR)/GPU/GPUEngine.o
endif

CXX      = g++
CUDA    ?= /usr/local/cuda
CXXCUDA  = /usr/bin/g++-4.8
NVCC     = $(CUDA)/bin/nvcc
# nvcc requires joint notation w/o dot, i.e. "5.2" -> "52"
ccap     = $(shell echo $(CCAP) | tr -d '.')

ifdef gpu
ifdef debug
CXXFLAGS = -DWITHGPU -m64 -mssse3 -Wno-write-strings -std=c++17 -g -MMD -MP -I. -I$(CUDA)/include
else
CXXFLAGS = -DWITHGPU -m64 -mssse3 -Wno-write-strings -std=c++17 -O2 -fno-strict-aliasing -MMD -MP -I. -I$(CUDA)/include
endif
LFLAGS   = -lpthread -L$(CUDA)/lib64 -lcudart
else
ifdef debug
CXXFLAGS = -m64 -mssse3 -Wno-write-strings -std=c++17 -g -MMD -MP -I.
else
CXXFLAGS = -m64 -mssse3 -Wno-write-strings -std=c++17 -O2 -fno-strict-aliasing -MMD -MP -I.
endif
LFLAGS   = -lpthread
endif

DEPS = $(OBJET:.o=.d)

#--------------------------------------------------------------------

.PHONY: all clean

all: VanitySearch

ifdef gpu
ifdef debug
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -G -maxrregcount=0 --ptxas-options=-v --compile --compiler-options -fPIC -ccbin $(CXXCUDA) -m64 -g -I$(CUDA)/include -gencode=arch=compute_$(ccap),code=sm_$(ccap) -o $@ -c $<
else
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -maxrregcount=0 --ptxas-options=-v --compile --compiler-options -fPIC -ccbin $(CXXCUDA) -m64 -O2 -I$(CUDA)/include -gencode=arch=compute_$(ccap),code=sm_$(ccap) -o $@ -c $<
endif
endif

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

VanitySearch: $(OBJET)
	@echo Making VanitySearch...
	$(CXX) $(OBJET) $(LFLAGS) -o VanitySearch

$(OBJET): | $(OBJDIR) $(OBJDIR)/GPU $(OBJDIR)/hash

$(OBJDIR) $(OBJDIR)/GPU $(OBJDIR)/hash:
	mkdir -p $@

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*.o $(OBJDIR)/*.d
	@rm -f $(OBJDIR)/GPU/*.o $(OBJDIR)/GPU/*.d
	@rm -f $(OBJDIR)/hash/*.o $(OBJDIR)/hash/*.d
	@rm -f VanitySearch

-include $(DEPS)
