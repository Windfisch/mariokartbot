all: detect_road_borders mariokart01

detect_road_borders: detect_road_borders.cpp
	g++ `pkg-config --libs --cflags opencv` $< -o $@

test_detect: detect_road_borders
	./detect_road_borders test.mpg

mariokart01: mariokart01.cpp
	g++ `pkg-config --libs --cflags opencv` -lxcb -lpthread $< -o $@

