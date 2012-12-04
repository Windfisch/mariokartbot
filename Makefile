.SUFFIXES: .cpp .o 
.PHONY: clean

all: detect_road_borders mariokart

clean:
	rm -f *.o mariokart detect_road_borders

.cpp.o:
	g++ `pkg-config --cflags opencv` -g -c $<

joystick.o: joystick.cpp os.h
	g++ `pkg-config --cflags opencv` -g -c $<

detect_road_borders: detect_road_borders.cpp
	g++ `pkg-config --libs --cflags opencv` -g $> -o $@

detect_road_borders_with_horizon_steerer: detect_road_borders_with_horizon_steerer.cpp horizon_steerer.o util.o
	g++ `pkg-config --libs --cflags opencv` -g $> -o $@

test_detect: detect_road_borders
	./detect_road_borders test.mpg


mariokart: mariokart.o os.h joystick.o xorg_grabber.o road_thresholder.o horizon_steerer.o naive_steerer.o steer_accumulator.o util.o
	g++ `pkg-config --libs --cflags opencv` -lxcb -lpthread -g $> -o $@

