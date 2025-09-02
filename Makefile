TARGET = mandelbrot.out
CXXFLAGS = -std=c++20 -march=native -O2 -ffast-math
# O3 is about the same as O2, and Ofast is probably completely fine

make:
	g++ -pthread -o $(TARGET) $(CXXFLAGS) main.cpp

plusplus:
	g++ -pthread -o $(TARGET) $(CXXFLAGS) -DUSE_MAGICK_PLUSPLUS main.cpp -lMagick++

enki:
	g++ -pthread -o $(TARGET) $(CXXFLAGS) -DUSE_ENKITS main.cpp enkiTS/TaskScheduler.cpp

enkiplusplus:
	g++ -pthread -o $(TARGET) $(CXXFLAGS) -DUSE_ENKITS -DUSE_MAGICK_PLUSPLUS main.cpp enkiTS/TaskScheduler.cpp -lMagick++

clean:
	rm -f $(TARGET)
