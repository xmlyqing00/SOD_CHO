
OPT_FLAGS = -O3

CXX = g++
CXX_FLAGS = $(OPT_FLAGS) --std=c++17 -Wall

OPENCV_LIBS = -I/usr/include/opencv -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_datasets -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_line_descriptor -lopencv_optflow -lopencv_video -lopencv_plot -lopencv_reg -lopencv_saliency -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_rgbd -lopencv_viz -lopencv_surface_matching -lopencv_text -lopencv_ximgproc -lopencv_calib3d -lopencv_features2d -lopencv_flann -lopencv_xobjdetect -lopencv_objdetect -lopencv_ml -lopencv_xphoto -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core

default: SRD_CHO

.PHONY : clean

SRD_CHO: main.o comman.o ncut.o cutobj.o evaluate.o pyramid.o saliency.o segment.o
	$(CXX) $^ $(OPENCV_LIBS) -o $@

%.o: %.cpp
	$(CXX) $^ $(CXX_FLAGS) -c

clean: 
	rm *.o SRD_CHO