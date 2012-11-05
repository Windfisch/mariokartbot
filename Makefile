all: detect_road_borders mariokart01
detect_road_borders: detect_road_borders.cpp
	g++ `pkg-config --libs --cflags opencv` $< -o $@

mariokart01: detect_road_borders.cpp
	g++ `pkg-config --libs --cflags opencv` -lxcb -lpthread $< -o $@

