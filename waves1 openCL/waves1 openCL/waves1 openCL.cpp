#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>
//#include <iostream>
#include <string>
#include <sstream>
#include <CL/cl.h>
#include <iostream>

#ifndef min
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif // min


using namespace sf;
using std::this_thread::sleep_for;

const int Width = 300;
const int Height = 300;

const int Screen_Scale = 2;
const int Screen_Width = Width * Screen_Scale;
const int Screen_Height = Height * Screen_Scale;


const double r = 0.1;//accuracy of simulation
const double fric_coef = 0.005;

//void openCl_compute();

void point_compute(double** field_y, double** field_y_prev, double** field_y_change, bool** field_is_wall, int x, int y);
void point_apply_changing(double** field_y, double** field_y_change, int x, int y);
void compute_frame(double** field_y, double** field_y_prev, double** field_y_change, bool** field_is_wall);

class Point {
private:
	double y_change;
public:
	bool is_wall = false;
	double y;
	double y_prev;

	Point(double y_init = 0.0) {
		this->y = y_init;
		this->y_prev = y_init;
	}
	~Point() {};

	//for ordinary points with 2 neighbors
	void Compute(const Point& point0, const Point& point1, const Point& point2, const Point& point3) {
		this->y_change = 2 * this->y - this->y_prev + r * (point0.y + point1.y + point2.y + point3.y - 4 * this->y);
		this->y_prev = this->y;
		this->y_change -= (this->y_change - this->y) * fric_coef;
	}

	void ApplyChanging() {
		this->y = this->y_change;
		//this->y_change = 0;
	}
};

class Field {
private:

public:
	Point** points;

	Field() {
		points = new Point * [Width];
		for (int x = 0; x < Width; x++) {
			points[x] = new Point[Height];
		}



		for (int x = 0; x < Width; x++) {
			for (int y = 0; y < Height; y++) {
				points[x][y].y = 0.0;
				points[x][y].y_prev = 0.0;
			}
		}

		for (int x = 0; x < Width; x++) {
			points[x][0].is_wall = true;
			points[x][Height - 1].is_wall = true;
		}
		for (int y = 0; y < Height; y++) {
			points[0][y].is_wall = true;
			points[Width - 1][y].is_wall = true;
		}
	}
	~Field() {
		for (int x = 0; x < Width; x++) {
			delete[] points[x];
			points[x] = nullptr;
		}
		delete[] points;
	};

	void ComputeFrame() {

		for (int i = 1; i < Width - 1; i++) {
			for (int j = 1; j < Height - 1; j++) {
				if (!points[i][j].is_wall) {
					points[i][j].Compute(points[i - 1][j - 1], points[i + 1][j - 1], points[i - 1][j + 1], points[i + 1][j + 1]);
				}

			}
		}

		for (int i = 1; i < Width - 1; i++) {
			for (int j = 1; j < Height - 1; j++) {
				if (!points[i][j].is_wall) {
					points[i][j].ApplyChanging();
				}
			}
		}
	}
};

int main()
{
#pragma region Create_variables
	double** field_y = new double* [Width];
	for (int i = 0; i < Width; i++) {
		field_y[i] = new double[Height];
	}

	double** field_y_prev = new double* [Width];
	for (int i = 0; i < Width; i++) {
		field_y_prev[i] = new double[Height];
	}

	double** field_y_temp = new double* [Width];
	for (int i = 0; i < Width; i++) {
		field_y_temp[i] = new double[Height];
	}

	bool** field_is_wall = new bool* [Width];
	for (int i = 0; i < Width; i++) {
		field_is_wall[i] = new bool[Height];
	}

#pragma endregion

#pragma region Init_variables

	for (int x = 0; x < Width; x++) {
		for (int y = 0; y < Height; y++) {
			if ((x == 0 || y == 0) || ((x == Width - 1) || (y == Height - 1))) field_is_wall[x][y] = true;
			else field_is_wall[x][y] = false;

			field_y[x][y] = 0;
			field_y_prev[x][y] = 0;
		}
	}

	

#pragma endregion

	

	const int offset = 50; //screen offset for text
	RenderWindow window(VideoMode(Screen_Width, Screen_Height + offset), "Wave simulation");
	window.setFramerateLimit(60);

	Uint8* pixels = new Uint8[4 * Screen_Width * Screen_Width];


	short brush_size = 3;
	double value = 3.0;
	bool cursor_enabled = true;

	bool C_flag = false;
	std::string outText = "";

	Font font;
	Text text;
	font.loadFromFile("D:\\Documents\\source\\repos\\Waves1\\arialmt.ttf"); //your path here
	text.setFont(font);

	text.setCharacterSize(23); // in pixels, not points!
	text.setFillColor(sf::Color::White);
	text.move(10.f, 5.f);
	std::ostringstream oss;

	oss << "Brush size: " << brush_size << "\tBrush value: " << value << "\tDraw mode enabled: " << cursor_enabled;
	outText = oss.str();
	text.setString(outText);

	/////////////////////////openCl_compute();

#pragma region Animation
	while (window.isOpen())
	{
		Event event;

		while (window.pollEvent(event))
		{
			int x = Mouse::getPosition(window).x;
			int y = Mouse::getPosition(window).y - offset;
			x = x * Width / window.getSize().x;
			y = y * Height / (window.getSize().y - offset);

			if (event.type == Event::Closed)
				window.close();

			if (cursor_enabled) {
				if (Mouse::isButtonPressed(Mouse::Left))
				{


					if (Keyboard::isKeyPressed(Keyboard::W)) { //if W pressed, add wall
						if ((x > brush_size && x < Width - brush_size) && (y > brush_size && y < Height - brush_size)) {

							for (int i = x - brush_size; i < x + brush_size; i++) {
								for (int j = y - brush_size; j < y + brush_size; j++) {
									field_is_wall[i][j] = true;

								}
							}

						}
					}
					else {//else set y of pixels to value
						if ((x > brush_size && x < Width - brush_size) && (y > brush_size && y < Height - brush_size)) {

							for (int i = x - brush_size; i < x + brush_size; i++) {
								for (int j = y - brush_size; j < y + brush_size; j++) {
									if (!field_is_wall[i][j]) {
										field_y[i][j] = value;
										field_y_prev[i][j] = value;
									}

								}
							}

						}
					}

				}

				if (Mouse::isButtonPressed(Mouse::Right)) //delete several walls
				{
					if ((x > brush_size && x < Width - brush_size) && (y > brush_size && y < Height - brush_size)) {

						for (int i = x - brush_size; i < x + brush_size; i++) {
							for (int j = y - brush_size; j < y + brush_size; j++) {
								field_is_wall[i][j] = false;
							}
						}

					}

				}
			}

			if (event.type == sf::Event::MouseWheelMoved)
			{
				if (Keyboard::isKeyPressed(Keyboard::B)) {
					brush_size += event.mouseWheel.delta;
					if (brush_size < 1) {
						brush_size = 1;
					}
				}
				if (Keyboard::isKeyPressed(Keyboard::V)) {
					value += event.mouseWheel.delta * 0.1;

				}

				oss.str(std::string());
				oss << "Brush size: " << brush_size << "\tBrush value: " << value << "\tDraw mode enabled: " << cursor_enabled;
				outText = oss.str();
				text.setString(outText);
			}

			if (Keyboard::isKeyPressed(Keyboard::C)) {
				if (C_flag) {
					C_flag = false;
					cursor_enabled = !cursor_enabled;
				}
				else {
					C_flag = true;
				}

				oss.str(std::string());
				oss << "Brush size: " << brush_size << "\tBrush value: " << value << "\tDraw mode enabled: " << cursor_enabled;
				outText = oss.str();
				text.setString(outText);
			}
		}

		//coloring of the screen
		for (int x = 0; x < Width; x++) {
			for (int y = 0; y < Height; y++) {
				int color, rcolor = 0, bcolor = 0, gcolor = 0;
				if (field_is_wall[x][y]) {
					rcolor = 255;
					gcolor = 255;
					bcolor = 255;
				}
				else {
					color = field_y[x][y] * 100;

					if (color > 0) {
						if (color > 255) {
							gcolor = color - 255;
							color = 255;
						}
						rcolor = color;
					}
					if (color <= 0) {
						color = -color;
						if (color > 255) {
							gcolor = color - 255;
							color = 255;
						}
						bcolor = color;
					}
				}

				for (int i = x * Screen_Scale; i < (x + 1) * Screen_Scale; i++) {
					for (int j = y * Screen_Scale; j < (y + 1) * Screen_Scale; j++) {
						int xScreen = i;
						int yScreen = j * Screen_Scale;
						pixels[(xScreen + yScreen * Width) * 4] = rcolor; //r
						pixels[(xScreen + yScreen * Width) * 4 + 1] = gcolor; //g
						pixels[(xScreen + yScreen * Width) * 4 + 2] = bcolor; //b
						pixels[(xScreen + yScreen * Width) * 4 + 3] = 255; //a
					}
				}

			}
		}

		//brush cursor
		if (cursor_enabled) {
			int xMouse = Mouse::getPosition(window).x;
			int yMouse = Mouse::getPosition(window).y - offset;
			xMouse = xMouse * Width / window.getSize().x;
			yMouse = yMouse * Height / (window.getSize().y - offset);

			if ((xMouse > brush_size && xMouse < Width - brush_size) && (yMouse > brush_size && yMouse < Height - brush_size)) {
				for (int x = (xMouse - brush_size) * Screen_Scale; x < (xMouse + brush_size) * Screen_Scale; x++) {
					pixels[(x + (yMouse + brush_size) * Width * Screen_Scale * Screen_Scale) * 4] = 127; //r
					pixels[(x + (yMouse + brush_size) * Width * Screen_Scale * Screen_Scale) * 4 + 1] = 127; //g
					pixels[(x + (yMouse + brush_size) * Width * Screen_Scale * Screen_Scale) * 4 + 2] = 127; //b
					pixels[(x + (yMouse + brush_size) * Width * Screen_Scale * Screen_Scale) * 4 + 3] = 255; //a

					pixels[(x + (yMouse - brush_size) * Width * Screen_Scale * Screen_Scale) * 4] = 127; //r
					pixels[(x + (yMouse - brush_size) * Width * Screen_Scale * Screen_Scale) * 4 + 1] = 127; //g
					pixels[(x + (yMouse - brush_size) * Width * Screen_Scale * Screen_Scale) * 4 + 2] = 127; //b
					pixels[(x + (yMouse - brush_size) * Width * Screen_Scale * Screen_Scale) * 4 + 3] = 255; //a
				}
				for (int y = (yMouse - brush_size) * Screen_Scale; y < (yMouse + brush_size) * Screen_Scale; y++) {
					pixels[((xMouse + brush_size) * Screen_Scale + y * Width * Screen_Scale) * 4] = 127; //r
					pixels[((xMouse + brush_size) * Screen_Scale + y * Width * Screen_Scale) * 4 + 1] = 127; //g
					pixels[((xMouse + brush_size) * Screen_Scale + y * Width * Screen_Scale) * 4 + 2] = 127; //b
					pixels[((xMouse + brush_size) * Screen_Scale + y * Width * Screen_Scale) * 4 + 3] = 255; //a

					pixels[((xMouse - brush_size) * Screen_Scale + y * Width * Screen_Scale) * 4] = 127; //r
					pixels[((xMouse - brush_size) * Screen_Scale + y * Width * Screen_Scale) * 4 + 1] = 127; //g
					pixels[((xMouse - brush_size) * Screen_Scale + y * Width * Screen_Scale) * 4 + 2] = 127; //b
					pixels[((xMouse - brush_size) * Screen_Scale + y * Width * Screen_Scale) * 4 + 3] = 255; //a
				}
			}
		}

		Image img;
		img.create(Screen_Width, Screen_Height, pixels);
		Texture texture;
		texture.loadFromImage(img);
		Sprite sprite;
		sprite.setTexture(texture, true);
		sprite.move(0.f, offset);
		window.clear();
		window.draw(text);
		window.draw(sprite);

		window.display();

		//sleep_for(std::chrono::milliseconds(10));
		compute_frame(field_y, field_y_prev, field_y_temp, field_is_wall);
	}
#pragma endregion
	
#pragma region Deletes

	delete[] pixels;

	for (int i = 0; i < Width; i++) {
		delete[] field_y[i];
	}
	delete[] field_y;

	for (int i = 0; i < Width; i++) {
		delete[] field_y_prev[i];
	}
	delete[] field_y_prev;

	for (int i = 0; i < Width; i++) {
		delete[] field_y_temp[i];
	}
	delete[] field_y_temp;

	for (int i = 0; i < Width; i++) {
		delete[] field_is_wall[i];
	}
	delete[] field_is_wall;
#pragma endregion

	return 0;
}


void point_compute(double** field_y, double** field_y_prev, double** field_y_change, bool** field_is_wall, int x, int y) {
	if (field_is_wall[x][y] == false) {   
		//field_y_change[x][y] = 2 * field_y[x][y] - field_y_prev[x][y] +   ////взаимодействие клетки с соседними "уголками"
		//	r * (field_y[x - 1][y - 1] + field_y[x + 1][y - 1] + field_y[x - 1][y + 1] + field_y[x + 1][y + 1] - 4 * field_y[x][y]);

		field_y_change[x][y] = 2 * field_y[x][y] - field_y_prev[x][y] +   ///взаимодействие клетки "квадратом"
			r * (field_y[x][y - 1] + field_y[x][y + 1] + field_y[x - 1][y] + field_y[x + 1][y] - 4 * field_y[x][y]);

		field_y_prev[x][y] = field_y[x][y];
		field_y_change[x][y] -= (field_y_change[x][y] - field_y[x][y]) * fric_coef;
	}
}

void point_apply_changing(double** field_y, double** field_y_change, int x, int y) {
	field_y[x][y] = field_y_change[x][y];
}

void compute_frame(double** field_y, double** field_y_prev, double** field_y_change, bool** field_is_wall) {
	for (int x = 0; x < Width; x++) {
		for (int y = 0; y < Height; y++) {
			point_compute(field_y, field_y_prev, field_y_change, field_is_wall, x, y);
		}
	}

	for (int x = 0; x < Width; x++) {
		for (int y = 0; y < Height; y++) {
			point_apply_changing(field_y ,field_y_change, x, y);
		}
	}
}




void openCl_compute() {
	cl_platform_id platform_id;
	cl_uint ret_num_platforms, ret_num_devices;
	cl_device_id device_id;
	cl_int ret;


	clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	/* получить доступные платформы */
	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);

	/* получить доступные устройства */
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	/* создать контекст */
	auto context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

	/* создаем команду */
	auto command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);

	//std::cout << "context: " << context << "\tcommand_queue: " << command_queue << '\n';


	cl_program program = NULL;
	cl_kernel kernel = NULL;

	FILE* fp;
	const char fileName[] = "kernel_test.cl";
	size_t source_size;
	char* source_str;
	int i;

#define MAX_SOURCE_SIZE 1024

	try {
		fopen_s(&fp, fileName, "r");
		// printf("fp is %f\n", fp);

		if (!fp) {
			fprintf(stderr, "Failed to load kernel.\n");
			exit(1);
		}
		source_str = (char*)malloc(MAX_SOURCE_SIZE);
		source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
		fclose(fp);
	}
	catch (int a) {
		printf("%i", a);
	}

	/* создать бинарник из кода программы */
	program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t*)&source_size, &ret);

	/* скомпилировать программу */
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	/* создать кернел */
	kernel = clCreateKernel(program, "test", &ret);
	delete[] source_str;

	std::cout << "ret: " << ret << '\n';

	cl_mem memobj = NULL;
	int memLenth = 10;
	cl_int* mem = (cl_int*)malloc(sizeof(cl_int) * memLenth);

	/* создать буфер */
	memobj = clCreateBuffer(context, CL_MEM_READ_WRITE, memLenth * sizeof(cl_int), NULL, &ret);

	/* записать данные в буфер */
	ret = clEnqueueWriteBuffer(command_queue, memobj, CL_TRUE, 0, memLenth * sizeof(cl_int), mem, 0, NULL, NULL);

	/* устанавливаем параметр */
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&memobj);




	size_t global_work_size[1] = { 10 };

	/* выполнить кернел */
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);

	/* считать данные из буфера */
	ret = clEnqueueReadBuffer(command_queue, memobj, CL_TRUE, 0, memLenth * sizeof(float), mem, 0, NULL, NULL);

	for (int i = 0; i < 10; i++) {
		std::cout << mem[i] << ' ';
	}
	std::cout << '\n';

	delete[] mem;
}
