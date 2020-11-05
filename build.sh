#!/bin/bash

echo "building hsv_picker..."

g++ -std=c++17 -Wall -L/usr/local/lib/ -O3 \
	-o hsv_picker hsv_picker.cpp \
	-lboost_system \
	-lopencv_highgui \
	-lopencv_imgcodecs \
	-lopencv_core \
	-lopencv_imgproc \
	-lpthread

echo "done."
