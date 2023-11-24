OPT_FLAGS = -O3
CXX = g++
INCLUDE_DIR = $(shell pkg-config --cflags opencv4)
CXX_FLAGS = $(OPT_FLAGS) $(INCLUDE_DIR) --std=c++17 -Wall
OPENCV_LIBS = $(shell pkg-config --libs opencv4)

default: SOD_CHO

.PHONY : clean

SOD_CHO: main.o comman.o ncut.o cutobj.o evaluate.o pyramid.o saliency.o segment.o
	$(CXX) $^ $(OPENCV_LIBS) -o $@

%.o: %.cpp
	$(CXX) $^ $(CXX_FLAGS) -c

clean: 
	rm *.o SOD_CHO