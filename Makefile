TARGET = mandelbrot.out
CXXFLAGS = -O2 -ffast-math
# O3 is about the same as O2, and Ofast is probably completely fine

make:
	g++ -pthread -o $(TARGET) $(CXXFLAGS) main.cpp

plusplus:
	g++ -pthread -o $(TARGET) $(CXXFLAGS) -DUSE_MAGICK_PLUSPLUS main.cpp -lMagick++

clean:
	rm -f $(TARGET)
