#include <complex>
#include <string>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint> //to be fancy with uint8_t vs uint16_t
#include <limits> //for <cstdint>

#include <cassert> //Magick++ makes its own assert (__assert_fail()), causes enkiTS to fail compilation
#include <Magick++.h>

#include "enkiTS/TaskScheduler.h"
enki::TaskScheduler g_TS;

typedef float c_float; //complex float precision

int MAX_ITER = 10000;
std::vector<std::pair<int, Magick::ColorRGB>> iterationColors = {
	//iteration count will always be >0
	{    1, Magick::ColorRGB(0, 0, 0) }, //black
	//{    4, Magick::ColorRGB(  0,   0, .01) }, //dark blue
	//{    5, Magick::ColorRGB(  0,   0, .05) }, //less dark blue
	//{   10, Magick::ColorRGB(  0, .25, .25) }, //quarter-turquoise
	{   15, Magick::ColorRGB(  0, .50, .50) }, //half-turquoise
	{   20, Magick::ColorRGB(  0,   1,   1) }, //full-turquoise //where it starts being close enough to the main pattern
	{   30, Magick::ColorRGB(  1,   1,   0) }, //yellow
	{  100, Magick::ColorRGB(  1, .75,   0) }, //yellow-orange
	{  500, Magick::ColorRGB(  1, .50,   0) }, //orange
	{ 1000, Magick::ColorRGB(  1, .25,   0) }, //orange-red
	{ 5000, Magick::ColorRGB(.25,   0,   0) }, //darker red
	{ MAX_ITER, Magick::ColorRGB(0, 0, 0) } //black
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
			r = std::stod(colorR);
			g = std::stod(colorG);
			b = std::stod(colorB);
			//double because Magick::ColorRGB takes doubles

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

struct MandelbrotTask : public enki::ITaskSet {
	#ifdef USE_IM6
	Magick::PixelPacket* pixel_arr;
	MandelbrotTask(Magick::PixelPacket* pixels, c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_width, int image_height);
	#else
	Magick::Quantum* pixel_arr;
	MandelbrotTask(Magick::Quantum* pixels, c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_width, int image_height);
	#endif

	c_float x_start, x_end, y_start, y_end;
	int image_width, image_height;

	void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override;
};

#ifdef USE_IM6
void mandelbrot_helper(c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_x_start, int image_x_end, int image_width, int image_y_start, int image_y_end, int image_height, Magick::PixelPacket* pixel_arr) {
#else
void mandelbrot_helper(c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_x_start, int image_x_end, int image_width, int image_y_start, int image_y_end, int image_height, Magick::Quantum* pixel_arr) {
#endif
	//flip y-range because images have the y-axis going down:
	y_start *= -1;
	y_end *= -1;
	std::swap(y_start, y_end);

	//now actually do the calculation:
	//std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
	for (int y = image_y_start; y < image_y_end; y++) {
		for (int x = image_x_start; x < image_x_end; x++) {
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

			#ifdef USE_IM6
			const int arr_pos = y * image_width + x;
			pixel_arr[arr_pos] = iterationColors[colorIndex].second;
			#else
			const int arr_pos = 3 * (y * image_width + x); //ColorRGB does not have an alpha channel
			pixel_arr[arr_pos + 0] = iterationColors[colorIndex].second.quantumRed();
			pixel_arr[arr_pos + 1] = iterationColors[colorIndex].second.quantumGreen();
			pixel_arr[arr_pos + 2] = iterationColors[colorIndex].second.quantumBlue();
			#endif
		}
	}
	//std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();

	//std::cout << "mandelbrot: " << "[" << image_y_start << "," << image_y_end << "] " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;
}

void mandelbrot(int threadCount, c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_width, int image_height, const std::string& output_filename) {
	//get image ready:

	Magick::Image generated_image;
	generated_image.size(Magick::Geometry(image_width, image_height));
	//generated_image.type(Magick::TrueColorType);
	generated_image.modifyImage();
	Magick::Pixels view(generated_image);
	#ifdef USE_IM6
	Magick::PixelPacket* pixel_arr = view.get(0, 0, image_width, image_height);
	//https://legacy.imagemagick.org/Magick++/Pixels.html
	#else
	Magick::Quantum* pixel_arr = view.get(0, 0, image_width, image_height);
	//https://imagemagick.org/Magick++/Pixels.html
	#endif

	//calculate mandelbrot:

	MandelbrotTask* mandelbrotTask = new MandelbrotTask(pixel_arr, x_start, x_end, y_start, y_end, image_width, image_height);
	std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
	g_TS.AddTaskSetToPipe(mandelbrotTask);
	g_TS.WaitforTask(mandelbrotTask);
	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();
	delete mandelbrotTask;
	std::cout << "mandelbrot: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;

	//write image:

	startTime = std::chrono::steady_clock::now();
	view.sync();
	generated_image.write(output_filename);
	endTime = std::chrono::steady_clock::now();
	std::cout << "write: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;
}

void MandelbrotTask::ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) {
	int image_x_start = 0;
	int image_x_end   = image_width;
	int image_y_start = range_.start;
	int image_y_end   = range_.end;
	mandelbrot_helper(x_start, x_end, y_start, y_end, image_x_start, image_x_end, image_width, image_y_start, image_y_end, image_height, pixel_arr);
}

#ifdef USE_IM6
MandelbrotTask::MandelbrotTask(Magick::PixelPacket* pixels, c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_width, int image_height) {
#else
MandelbrotTask::MandelbrotTask(Magick::Quantum* pixels, c_float x_start, c_float x_end, c_float y_start, c_float y_end, int image_width, int image_height) {
#endif
	m_MinRange = 1; //smaller ranges don't help tiny images, but they slightly help very large images
	m_SetSize = image_height;
	pixel_arr = pixels;
	this->x_start = x_start;
	this->x_end = x_end;
	this->y_start = y_start;
	this->y_end = y_end;
	this->image_width = image_width;
	this->image_height = image_height;
}



int main(int argc, char** argv) {
	if (argc < 9) {
		std::cout << "usage: " << argv[0] << " <num_threads> <x_start> <x_end> <y_start> <y_end> <image_x_size> <image_y_size> <output_name> [<optional coloring file>]" << std::endl;
		return 1;
	}
	Magick::InitializeMagick(argv[0]);

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

	g_TS.Initialize(threadCount);

	std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();

	mandelbrot(threadCount, x_start, x_end, y_start, y_end, image_width, image_height, output_filename);

	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();

	std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "ms" << std::endl;
}
