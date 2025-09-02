#include <string>
#include <chrono>
#include <thread>
#include <future>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <complex>
#include <vector>
#include <cstdint> //to be fancy with uint8_t vs uint16_t
#include <limits> //for <cstdint>
#include <cstdio> //could use <filesystem> but I just want remove()

typedef float c_float; //complex float precision
typedef uint8_t color_type;
constexpr color_type COLOR_MAX_VAL = std::numeric_limits<color_type>::max();

#ifdef USE_MAGICK_PLUSPLUS
#include <Magick++.h>
#else
#include <cstdlib> //for system() to interact with ImageMagick
inline int ImageMagickConvert(const std::string& output_filename) {
	#ifdef WIN32
	return std::system(std::string("magick "  + (output_filename + ".txt") + " " + output_filename).c_str());
	#else
	return std::system(std::string("convert " + (output_filename + ".txt") + " " + output_filename).c_str());
	#endif
}
inline std::string getImageMagickTextImageHeader(int image_width, int image_height) {
	return "# ImageMagick pixel enumeration: " + std::to_string(image_width) + "," + std::to_string(image_height) + "," + std::to_string(COLOR_MAX_VAL) + ",srgb";
}
#endif

#ifdef USE_ENKITS
#include "enkiTS/TaskScheduler.h"
enki::TaskScheduler g_TS;
#endif

struct ColorRGB {
protected:
	color_type values[4]; //alpha not used, it's just padding
public:
	ColorRGB(float r, float g, float b) {
		values[0] = r * COLOR_MAX_VAL;
		values[1] = g * COLOR_MAX_VAL;
		values[2] = b * COLOR_MAX_VAL;
		//not bothering to error check bounds (use std::clamp if you want to)
	}
	ColorRGB() {} //just so things compile
	inline color_type getR() const noexcept { return values[0]; }
	inline color_type getG() const noexcept { return values[1]; }
	inline color_type getB() const noexcept { return values[2]; }
};

struct ImagePixel {
protected:
	ColorRGB color;
	int xpos, ypos;
public:
	ImagePixel(int x, int y, const ColorRGB& c) : color(c) {
		//color = c;
		xpos = x;
		ypos = y;
	}
	ImagePixel() {} //just so things compile

	std::string toString() const {
		return std::to_string(xpos) + "," + std::to_string(ypos) + ": (" + std::to_string(color.getR()) + "," + std::to_string(color.getG()) + "," + std::to_string(color.getB()) + ")";
	}
	inline c_float getRf() const noexcept { return color.getR() / COLOR_MAX_VAL; }
	inline c_float getGf() const noexcept { return color.getG() / COLOR_MAX_VAL; }
	inline c_float getBf() const noexcept { return color.getB() / COLOR_MAX_VAL; }
};

int MAX_ITER = 10000;
std::vector<std::pair<int, ColorRGB>> iterationColors = {
	//iteration count will always be >0
	{    1, ColorRGB(0, 0, 0) }, //black
	//{    4, ColorRGB(  0,   0, .01) }, //dark blue
	//{    5, ColorRGB(  0,   0, .05) }, //less dark blue
	//{   10, ColorRGB(  0, .25, .25) }, //quarter-turquoise
	{   15, ColorRGB(  0, .50, .50) }, //half-turquoise
	{   20, ColorRGB(  0,   1,   1) }, //full-turquoise //where it starts being close enough to the main pattern
	{   30, ColorRGB(  1,   1,   0) }, //yellow
	{  100, ColorRGB(  1, .75,   0) }, //yellow-orange
	{  500, ColorRGB(  1, .50,   0) }, //orange
	{ 1000, ColorRGB(  1, .25,   0) }, //orange-red
	{ 5000, ColorRGB(.25,   0,   0) }, //darker red
	{ MAX_ITER, ColorRGB(0, 0, 0) } //black
};
//idea: option for linear interpolation for color boundaries

void readColorFileAndSetColors(const std::string& filename) {
	std::ifstream coloringFile;
	coloringFile.open(filename);
	if (coloringFile.is_open()) {
		iterationColors.clear();

		std::string line;
		int lineNum = 0;
		while (std::getline(coloringFile, line)) {
			lineNum++;

			//clear left and right whitespace:

			size_t space_pos = line.find_first_not_of(" \t");
			line.erase(0, space_pos);
			space_pos = line.find_last_not_of(" \t\r"); //getline() goes to the \n, leaving the \r
			line.erase(space_pos+1);

			if (line.size() == 0) [[unlikely]] {
				continue;
			}

			//get the values as strings:

			space_pos = line.find_first_of(" \t");
			if (space_pos == std::string::npos) {
				throw std::runtime_error("Syntax error reading line " + std::to_string(lineNum) + ": not enough space characters");
			}
			std::string iterations = line.substr(0, space_pos);
			space_pos = line.find_first_not_of(" \t", space_pos);
			line.erase(0, space_pos);

			space_pos = line.find_first_of(" \t");
			if (space_pos == std::string::npos) {
				throw std::runtime_error("Syntax error reading line " + std::to_string(lineNum) + ": not enough space characters");
			}
			std::string colorR = line.substr(0, space_pos);
			space_pos = line.find_first_not_of(" \t", space_pos);
			line.erase(0, space_pos);

			space_pos = line.find_first_of(" \t");
			if (space_pos == std::string::npos) {
				throw std::runtime_error("Syntax error reading line " + std::to_string(lineNum) + ": not enough space characters");
			}
			std::string colorG = line.substr(0, space_pos);
			space_pos = line.find_first_not_of(" \t", space_pos);
			line.erase(0, space_pos);
			std::string colorB = line; //don't bother checking for anything else

			//convert to numbers and push to list:

			int iter; float r, g, b;
			iter = std::stoi(iterations);
			r = std::stof(colorR);
			g = std::stof(colorG);
			b = std::stof(colorB);

			iterationColors.push_back({ iter, {r, g, b} });
		}
		if (iterationColors.empty()) [[unlikely]] {
			throw std::runtime_error("Syntax error: nothing in \"" + filename + "\"");
		}
		MAX_ITER = iterationColors[iterationColors.size()-1].first;

		//handling the file not being sorted for some reason:
		//std::stable_sort(iterationColors.begin(), iterationColors.end(),
		//	[](const std::pair<int, ColorRGB>& lhs, const std::pair<int, ColorRGB>& rhs) { return lhs.first < rhs.first; });
		//should be stable to give priority coloring to later lines

		iterationColors.shrink_to_fit();
		coloringFile.close();
	} else {
		throw std::runtime_error("Could not open file \"" + filename + "\"");
	}
}

void mandelbrot_helper(c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_x_start, int image_x_end, int image_width, int image_y_start, int image_y_end, int image_height, ImagePixel** pixelGrid) {
	//flip y-range because images have the y-axis going down:
	y_start *= -1;
	y_end *= -1;
	std::swap(y_start, y_end);

	//now actually do the calculation:
	//std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
	for (int x = image_x_start; x < image_x_end; x++) {
		for (int y = image_y_start; y < image_y_end; y++) {
			//using the center of the pixel
			const c_float pointX = ((c_float(x)+c_float(.5)) * (x_end - x_start)) / (image_width)  + x_start;
			const c_float pointY = ((c_float(y)+c_float(.5)) * (y_end - y_start)) / (image_height) + y_start;

			int iterations = 0;
			std::complex<c_float> z = std::complex<c_float>(0, 0);
			std::complex<c_float> c = std::complex<c_float>(pointX, pointY);
			while (std::norm(z) < 2*2 && iterations < MAX_ITER) {
				z = z*z + c;
				iterations++;
			}

			//color lookup
			int colorIndex = 0;
			for (int i = 1; i < iterationColors.size(); i++) {
				if (iterations >= iterationColors[i].first) {
					colorIndex = i;
				} else {
					break;
				}
			}
			pixelGrid[x][y] = ImagePixel(x, y, iterationColors[colorIndex].second);
		}
	}
	//std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();

	//std::cout << "mandelbrot: " << "[" << image_y_start << "," << image_y_end << "] " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;
}

#ifdef USE_ENKITS
struct MandelbrotTask : public enki::ITaskSet {
	ImagePixel** pixelsGrid;
	c_float x_start, x_end, y_start, y_end;
	int image_width, image_height;

	void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override {
		int image_x_start = range_.start;
		int image_x_end   = range_.end;
		int image_y_start = 0;
		int image_y_end   = image_height;
		mandelbrot_helper(x_start, x_end, y_start, y_end, image_x_start, image_x_end, image_width, image_y_start, image_y_end, image_height, pixelsGrid);
	}

	MandelbrotTask(ImagePixel** pixels, c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_width, int image_height) {
		m_MinRange = 16; //random guess; using 1 might net a tiny gain though
		m_SetSize = image_width;
		pixelsGrid = pixels;
		this->x_start = x_start;
		this->x_end = x_end;
		this->y_start = y_start;
		this->y_end = y_end;
		this->image_width = image_width;
		this->image_height = image_height;
	}
	~MandelbrotTask() override {
		//nothing
	}
};
#endif

void mandelbrot(int threadCount, c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_width, int image_height, std::string output_filename) {
	//pixel grid to modify:

	ImagePixel** pixels = new ImagePixel*[image_width];
	for (int i = 0; i < image_width; i++) {
		pixels[i] = new ImagePixel[image_height];
	}

	//main stuff:

	#ifdef USE_ENKITS

	MandelbrotTask* mandelbrotTask = new MandelbrotTask(pixels, x_start, x_end, y_start, y_end, image_width, image_height);
	std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
	g_TS.AddTaskSetToPipe(mandelbrotTask);
	g_TS.WaitforTask(mandelbrotTask);
	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();
	delete mandelbrotTask;

	#else

	std::future<void>* results = new std::future<void>[threadCount-1];
	std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
	for (int i = 0; i < threadCount-1; i++) {
		int image_x_start = (i)  /float(threadCount) * image_width;
		int image_x_end   = (i+1)/float(threadCount) * image_width;
		int image_y_start = 0;
		int image_y_end   = image_height;
		results[i] = std::async(std::launch::async, mandelbrot_helper, x_start, x_end, y_start, y_end, image_x_start, image_x_end, image_width, image_y_start, image_y_end, image_height, pixels);
		//not bothering for error handling on thread creation
	}
	mandelbrot_helper(x_start, x_end, y_start, y_end, (threadCount-1)/float(threadCount) * image_width, image_width, image_width, 0, image_height, image_height, pixels);

	for (int i = 0; i < threadCount-1; i++) {
		results[i].get();
	}
	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();
	delete[] results;

	#endif

	std::cout << "mandelbrot: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;

	#ifdef USE_MAGICK_PLUSPLUS

	startTime = std::chrono::steady_clock::now();
	Magick::Image generated_image;
	generated_image.size(Magick::Geometry(image_width, image_height));
	for (int i = 0; i < image_width; i++) {
		for (int j = 0; j < image_height; j++) {
			generated_image.pixelColor(i, j, Magick::ColorRGB(pixels[i][j].getRf(), pixels[i][j].getGf(), pixels[i][j].getBf()));
		}
	}

	generated_image.write(output_filename);
	endTime = std::chrono::steady_clock::now();
	std::cout << "magick++: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;

	#else

	//text file for ImageMagick:
	//could be multi-threaded, but this step usually takes <10% of the total time

	std::ofstream tempImageFile;
	tempImageFile.open(output_filename + ".txt");
	if (tempImageFile.is_open()) {
		startTime = std::chrono::steady_clock::now();

		tempImageFile << (getImageMagickTextImageHeader(image_width, image_height) + "\n");
		std::string pixelAccumulation = "";
		for (int i = 0; i < image_width; i++) {
			for (int j = 0; j < image_height; j++) {
				pixelAccumulation += (pixels[i][j].toString() + "\n");
				//would std::accumulate be faster?
			}
		}
		tempImageFile << pixelAccumulation;
		tempImageFile.close();

		endTime = std::chrono::steady_clock::now();
		std::cout << "stringify: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;
	} else {
		throw std::runtime_error("Could not open file \"" + (output_filename + ".txt") + "\"");
	}

	//magick convert:

	startTime = std::chrono::steady_clock::now();
	ImageMagickConvert(output_filename);
	endTime = std::chrono::steady_clock::now();
	std::cout << "convert: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;

	std::remove(std::string(output_filename + ".txt").c_str());

	#endif

	//free:

	for (int i = 0; i < image_width; i++) {
		delete[] pixels[i];
	}
	delete[] pixels;
}



int main(int argc, char** argv) {
	if (argc < 9) {
		std::cout << "usage: " << argv[0] << " <num_threads> <x_start> <x_end> <y_start> <y_end> <image_x_size> <image_y_size> <output_name> [<optional coloring file>]" << std::endl;
		return 1;
	}
	#ifdef USE_MAGICK_PLUSPLUS
	Magick::InitializeMagick(argv[0]);
	#endif

	int threadCount; //std::thread::hardware_concurrency() exists but there's no need to use it
	c_float x_start, x_end, y_start, y_end;
	int image_width, image_height;
	std::string output_filename;
	std::string coloring_filename;

	threadCount = std::stoi(std::string(argv[1]));
	threadCount = (threadCount < 1) ? 1 : threadCount;
	x_start = std::stold(std::string(argv[2]));
	x_end   = std::stold(std::string(argv[3]));
	y_start = std::stold(std::string(argv[4]));
	y_end   = std::stold(std::string(argv[5]));
	image_width  = std::stoi(std::string(argv[6]));
	image_height = std::stoi(std::string(argv[7]));
	output_filename = std::string(argv[8]);

	if (argc >= 10) {
		coloring_filename = std::string(argv[9]);
	} else {
		coloring_filename = "";
	}
	if (coloring_filename.size() > 0) {
		readColorFileAndSetColors(coloring_filename);
	}

	#ifdef USE_ENKITS
	g_TS.Initialize(threadCount);
	#endif

	std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();

	mandelbrot(threadCount, x_start, x_end, y_start, y_end, image_width, image_height, output_filename);

	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();

	std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;
}
