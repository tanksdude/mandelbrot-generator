CXX = g++
TARGET = mandelbrot.out
CXXFLAGS = -std=c++20 -march=native -O3 -ffast-math
# O3 *slightly* faster than O2, and Ofast is probably completely fine
MAGICK_FLAGS = $(shell pkg-config --cflags --libs Magick++)
CXXFLAGS += -DUSE_IM6 # IMPORTANT: If you have ImageMagick 7, remove this!

make:
	$(CXX) -pthread -o $(TARGET) $(CXXFLAGS) main.cpp enkiTS/TaskScheduler.cpp $(MAGICK_FLAGS)

clean:
	rm -f $(TARGET)
