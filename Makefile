all: detect_road_borders mariokart01

detect_road_borders: detect_road_borders.cpp
	g++ `pkg-config --libs --cflags opencv` -g $< -o $@

road_thresholder.o: road_thresholder.cpp road_thresholder.h
	g++ `pkg-config --cflags opencv` -g road_thresholder.cpp -c

horizon_steerer.o: horizon_steerer.cpp horizon_steerer.h
	g++ `pkg-config --cflags opencv` -g horizon_steerer.cpp -c

naive_steerer.o: naive_steerer.cpp naive_steerer.h
	g++ `pkg-config --cflags opencv` -g naive_steerer.cpp -c

steer_accumulator.o: steer_accumulator.cpp steer_accumulator.h
	g++ `pkg-config --cflags opencv` -g steer_accumulator.cpp -c

util.o: util.cpp util.h
	g++ `pkg-config --cflags opencv` -g util.cpp -c

test_detect: detect_road_borders
	./detect_road_borders test.mpg

mariokart01.o: mariokart01.cpp
	g++ `pkg-config --cflags opencv` -lxcb -lpthread -g mariokart01.cpp -c

mariokart01: mariokart01.o road_thresholder.o horizon_steerer.o naive_steerer.o steer_accumulator.o util.o
	g++ `pkg-config --libs --cflags opencv` -lxcb -lpthread -g mariokart01.o road_thresholder.o horizon_steerer.o naive_steerer.o steer_accumulator.o util.o -o $@

