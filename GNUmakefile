.PHONY: clean

all: detect_road_borders mariokart

clean:
	rm -f *.o mariokart detect_road_borders

.cpp.o:
	g++ `pkg-config --cflags opencv` -g -c $<

detect_road_borders: detect_road_borders.cpp
	g++ `pkg-config --libs --cflags opencv` -g detect_road_borders.cpp -o detect_road_borders

test_detect: detect_road_borders
	./detect_road_borders test.mpg


mariokart: mariokart.cpp os.h joystick.cpp xorg_grabber.cpp road_thresholder.cpp horizon_steerer.cpp naive_steerer.cpp steer_accumulator.cpp util.cpp
	g++ `pkg-config --libs --cflags opencv` -lxcb -lpthread -g mariokart.cpp os.h joystick.cpp xorg_grabber.cpp road_thresholder.cpp horizon_steerer.cpp naive_steerer.cpp steer_accumulator.cpp util.cpp -o mariokart

