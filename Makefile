CXX = g++
TARGET = mandelbrot.out
CXXFLAGS = -std=c++20 -march=native -O3 -ffast-math
# O3 *slightly* faster than O2, and Ofast is probably completely fine
MAGICK_FLAGS = $(shell pkg-config --cflags --libs Magick++)

make:
	$(CXX) -pthread -o $(TARGET) $(CXXFLAGS) main.cpp

plusplus:
	$(CXX) -pthread -o $(TARGET) $(CXXFLAGS) -DUSE_MAGICK_PLUSPLUS main.cpp $(MAGICK_FLAGS)

enki:
	$(CXX) -pthread -o $(TARGET) $(CXXFLAGS) -DUSE_ENKITS main.cpp enkiTS/TaskScheduler.cpp

enkiplusplus:
	$(CXX) -pthread -o $(TARGET) $(CXXFLAGS) -DUSE_ENKITS -DUSE_MAGICK_PLUSPLUS main.cpp enkiTS/TaskScheduler.cpp $(MAGICK_FLAGS)

clean:
	rm -f $(TARGET)
