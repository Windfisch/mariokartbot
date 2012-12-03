all: detect_road_borders mariokart01

detect_road_borders: detect_road_borders.cpp
	g++ `pkg-config --libs --cflags opencv` -g $< -o $@

horizon_steerer.o: horizon_steerer.cpp horizon_steerer.h
	g++ `pkg-config --libs --cflags opencv` -g horizon_steerer.cpp -c

steer_accumulator.o: steer_accumulator.cpp steer_accumulator.h
	g++ `pkg-config --libs --cflags opencv` -g steer_accumulator.cpp -c

test_detect: detect_road_borders
	./detect_road_borders test.mpg

mariokart01: mariokart01.cpp
	g++ `pkg-config --libs --cflags opencv` -lxcb -lpthread -g $< -o $@

