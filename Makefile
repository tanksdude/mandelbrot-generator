TARGET = mandelbrot.out
CXXFLAGS = -O2 -ffast-math
# O3 is about the same as O2, and Ofast is probably completely fine

make:
	g++ -pthread -o $(TARGET) $(CXXFLAGS) main.cpp

clean:
	rm -f $(TARGET)
