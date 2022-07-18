#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>
//#include <iostream>
#include <string>
#include <sstream>
#include <CL/cl.h>
#include <iostream>
#include <map>

#ifndef min
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif // min


using namespace sf;
using std::this_thread::sleep_for;

const int Width = 300;
const int Height = 300;

const int divider = 15; //Width and Height must be divisible by divider

const cl_int x_cell_size = Width / divider;
const cl_int y_cell_size = Height / divider;

const int Screen_Scale = 2;
const int Screen_Width = Width * Screen_Scale;
const int Screen_Height = Height * Screen_Scale;


const float r = 0.1;//accuracy of simulation
const float fric_coef = 0.005;



void openCl_compute(float*& field_y, float*& field_y_prev, float*& field_y_change, bool*& field_is_wall,
	std::map<int, cl_mem>& buffers, cl_kernel& kernel_compute, cl_kernel& kernel_apply, cl_command_queue& command_queue);

void point_compute(float* field_y, float* field_y_prev, float* field_y_change, bool* field_is_wall, int x, int y);
void point_apply_changing(float* field_y, float* field_y_change, int x, int y);
void compute_frame(float* field_y, float* field_y_prev, float* field_y_change, bool* field_is_wall);

void frame_apply_changing(float* field_y, float* field_y_prev, float* field_y_change, bool* field_is_wall) {
	for (int x = 0; x < Width; x++) {
		for (int y = 0; y < Height; y++) {
			if (field_is_wall[x + Width * y] == false) {
				field_y[x + Width * y] = field_y_change[x + Width * y];
			}
		}
	}
}

int main()
{
#pragma region Create_variables
	float* field_y = new float [Width * Height];

	float* field_y_prev = new float[Width * Height];

	float* field_y_change = new float[Width * Height];

	bool* field_is_wall = new bool[Width * Height];

#pragma endregion

#pragma region Init_variables

	for (int x = 0; x < Width; x++) {
		for (int y = 0; y < Height; y++) {
			if ((x == 0 || y == 0) || ((x == Width - 1) || (y == Height - 1))) field_is_wall[x + Width * y] = true;
			else field_is_wall[x + Width * y] = false;

			field_y[x + Width * y] = 0;
			field_y_prev[x + Width * y] = 0;
		}
	}

	

#pragma endregion


#pragma region Kernel
	cl_platform_id platform_id;
	cl_uint ret_num_platforms, ret_num_devices;
	cl_device_id device_id;
	cl_int ret;


	clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);

	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

	cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

	cl_command_queue command_queue = clCreateCommandQueueWithProperties(context, device_id, 0, &ret);


	cl_program program = NULL;
	cl_kernel kernel_compute = NULL, kernel_apply = NULL;

	FILE* fp;
	const char fileName[] = "kernel_compute_frame.cl";
	size_t source_size;
	char* source_str;

#define MAX_SOURCE_SIZE 10000

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

	program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t*)&source_size, &ret);
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	kernel_compute = clCreateKernel(program, "compute", &ret);
	std::cout << "ret: " << ret << '\n';
	kernel_apply = clCreateKernel(program, "apply", &ret);
	std::cout << "ret: " << ret << '\n';

	free(source_str);

	cl_mem field_y_to_kernel, field_y_prev_to_kernel, field_y_change_to_kernel, field_is_wall_to_kernel;
	cl_mem x_cell_size_kernel, y_cell_size_kernel, r_kernel, fric_kernel;

	size_t global_work_size[2] = { divider, divider };

	field_y_to_kernel = clCreateBuffer(context, CL_MEM_READ_WRITE, Width * Height * sizeof(float), NULL, &ret);
	field_y_prev_to_kernel = clCreateBuffer(context, CL_MEM_READ_WRITE, Width * Height * sizeof(float), NULL, &ret);
	field_y_change_to_kernel = clCreateBuffer(context, CL_MEM_READ_WRITE, Width * Height * sizeof(float), NULL, &ret);
	field_is_wall_to_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, Width * Height * sizeof(bool), NULL, &ret);
	x_cell_size_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_int), NULL, &ret);
	y_cell_size_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_int), NULL, &ret);
	r_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float), NULL, &ret);
	fric_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float), NULL, &ret);

	std::map<int, cl_mem> buffers = {
		{0, field_y_to_kernel},
		{1, field_y_prev_to_kernel},
		{2, field_y_change_to_kernel},
		{3, field_is_wall_to_kernel},
		{4, x_cell_size_kernel},
		{5, y_cell_size_kernel},
		{6, r_kernel},
		{7, fric_kernel}
	};

	/*
	ret = clEnqueueWriteBuffer(command_queue, field_y_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, field_y_change_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, field_y_prev_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y_prev, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, field_is_wall_to_kernel, CL_TRUE, 0, Width * Height * sizeof(bool), field_is_wall, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, x_cell_size_kernel, CL_TRUE, 0, sizeof(cl_int), &x_cell_size, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, y_cell_size_kernel, CL_TRUE, 0, sizeof(cl_int), &y_cell_size, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, r_kernel, CL_TRUE, 0, sizeof(float), &r, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, fric_kernel, CL_TRUE, 0, sizeof(float), &fric_coef, 0, NULL, NULL);


	ret = clSetKernelArg(kernel_compute, 0, sizeof(cl_mem), (void*)&field_y_to_kernel);
	ret = clSetKernelArg(kernel_compute, 1, sizeof(cl_mem), (void*)&field_y_prev_to_kernel);
	ret = clSetKernelArg(kernel_compute, 2, sizeof(cl_mem), (void*)&field_y_change_to_kernel);
	ret = clSetKernelArg(kernel_compute, 3, sizeof(cl_mem), (void*)&field_is_wall_to_kernel);
	ret = clSetKernelArg(kernel_compute, 4, sizeof(cl_mem), (void*)&x_cell_size_kernel);
	ret = clSetKernelArg(kernel_compute, 5, sizeof(cl_mem), (void*)&y_cell_size_kernel);
	ret = clSetKernelArg(kernel_compute, 6, sizeof(cl_mem), (void*)&r_kernel);
	ret = clSetKernelArg(kernel_compute, 7, sizeof(cl_mem), (void*)&fric_kernel);


	ret = clEnqueueNDRangeKernel(command_queue, kernel_compute, 2, NULL, global_work_size, NULL, 0, NULL, NULL); ///////////////


	ret = clEnqueueReadBuffer(command_queue, field_y_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	ret = clEnqueueReadBuffer(command_queue, field_y_prev_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y_prev, 0, NULL, NULL);
	ret = clEnqueueReadBuffer(command_queue, field_y_change_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
	ret = clEnqueueReadBuffer(command_queue, field_is_wall_to_kernel, CL_TRUE, 0, Width * Height * sizeof(bool), field_is_wall, 0, NULL, NULL);
	*/
#pragma endregion


	const int offset = 50; //screen offset for text
	RenderWindow window(VideoMode(Screen_Width, Screen_Height + offset), "Wave simulation");
	window.setFramerateLimit(60);

	Uint8* pixels = new Uint8[4 * Screen_Width * Screen_Width];


	short brush_size = 3;
	float value = 3.0;
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

	//openCl_compute(field_y, field_y_prev, field_y_change, field_is_wall);

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
									field_is_wall[i + j * Width] = true;

								}
							}

						}
					}
					else {//else set y of pixels to value
						if ((x > brush_size && x < Width - brush_size) && (y > brush_size && y < Height - brush_size)) {

							for (int i = x - brush_size; i < x + brush_size; i++) {
								for (int j = y - brush_size; j < y + brush_size; j++) {
									if (!field_is_wall[i + j * Width]) {
										field_y[i + j * Width] = value;
										field_y_prev[i + j * Width] = value;
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
								field_is_wall[i + j * Width] = false;
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
				if (field_is_wall[x + Width * y]) {
					rcolor = 255;
					gcolor = 255;
					bcolor = 255;
				}
				else {
					color = field_y[x + Width * y] * 100;

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


		/*
		
		ret = clEnqueueWriteBuffer(command_queue, field_y_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
		ret = clEnqueueWriteBuffer(command_queue, field_y_change_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
		ret = clEnqueueWriteBuffer(command_queue, field_y_prev_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y_prev, 0, NULL, NULL);
		ret = clEnqueueWriteBuffer(command_queue, field_is_wall_to_kernel, CL_TRUE, 0, Width * Height * sizeof(bool), field_is_wall, 0, NULL, NULL);
		ret = clEnqueueWriteBuffer(command_queue, x_cell_size_kernel, CL_TRUE, 0, sizeof(cl_int), &x_cell_size, 0, NULL, NULL);
		ret = clEnqueueWriteBuffer(command_queue, y_cell_size_kernel, CL_TRUE, 0, sizeof(cl_int), &y_cell_size, 0, NULL, NULL);
		ret = clEnqueueWriteBuffer(command_queue, r_kernel, CL_TRUE, 0, sizeof(float), &r, 0, NULL, NULL);
		ret = clEnqueueWriteBuffer(command_queue, fric_kernel, CL_TRUE, 0, sizeof(float), &fric_coef, 0, NULL, NULL);


		ret = clSetKernelArg(kernel_compute, 0, sizeof(cl_mem), (void*)&field_y_to_kernel);
		ret = clSetKernelArg(kernel_compute, 1, sizeof(cl_mem), (void*)&field_y_prev_to_kernel);
		ret = clSetKernelArg(kernel_compute, 2, sizeof(cl_mem), (void*)&field_y_change_to_kernel);
		ret = clSetKernelArg(kernel_compute, 3, sizeof(cl_mem), (void*)&field_is_wall_to_kernel);
		ret = clSetKernelArg(kernel_compute, 4, sizeof(cl_mem), (void*)&x_cell_size_kernel);
		ret = clSetKernelArg(kernel_compute, 5, sizeof(cl_mem), (void*)&y_cell_size_kernel);
		ret = clSetKernelArg(kernel_compute, 6, sizeof(cl_mem), (void*)&r_kernel);
		ret = clSetKernelArg(kernel_compute, 7, sizeof(cl_mem), (void*)&fric_kernel);


		ret = clEnqueueNDRangeKernel(command_queue, kernel_compute, 2, NULL, global_work_size, NULL, 0, NULL, NULL); ///////////////


		ret = clEnqueueReadBuffer(command_queue, field_y_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
		ret = clEnqueueReadBuffer(command_queue, field_y_prev_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y_prev, 0, NULL, NULL);
		ret = clEnqueueReadBuffer(command_queue, field_y_change_to_kernel, CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
		ret = clEnqueueReadBuffer(command_queue, field_is_wall_to_kernel, CL_TRUE, 0, Width * Height * sizeof(bool), field_is_wall, 0, NULL, NULL);
		*/

		openCl_compute(field_y, field_y_prev, field_y_change, field_is_wall, buffers, kernel_compute, kernel_apply, command_queue);


		//sleep_for(std::chrono::milliseconds(10));
		//compute_frame(field_y, field_y_prev, field_y_change, field_is_wall);
		//frame_apply_changing(field_y, field_y_prev, field_y_change, field_is_wall);
	}
#pragma endregion
	
#pragma region Deletes

	delete[] pixels;

	delete[] field_y;
	delete[] field_y_prev;
	delete[] field_y_change;
	delete[] field_is_wall;
#pragma endregion

	return 0;
}


void point_compute(float* field_y, float* field_y_prev, float* field_y_change, bool* field_is_wall, int x, int y) {
	if (field_is_wall[x + Width * y] == false) {   
		//field_y_change[x + Width * y] = 2 * field_y[x + Width * y] - field_y_prev[x + Width * y] +   ////взаимодействие клетки с соседними "уголками"
		//	r * (field_y[x - 1][y - 1] + field_y[x + 1][y - 1] + field_y[x - 1][y + 1] + field_y[x + 1][y + 1] - 4 * field_y[x + Width * y]);

		field_y_change[x + Width * y] = 2 * field_y[x + Width * y] - field_y_prev[x + Width * y] +   ///взаимодействие клетки "квадратом"
			r * (field_y[(x) + Width * (y - 1)] + field_y[(x)+Width * (y + 1)] + field_y[(x - 1)+Width * (y)] + field_y[(x + 1) + Width * (y)] - 4 * field_y[(x) + Width * (y)]);

		field_y_prev[x + Width * y] = field_y[x + Width * y];
		field_y_change[x + Width * y] -= (field_y_change[x + Width * y] - field_y[x + Width * y]) * fric_coef;
	}
}

void point_apply_changing(float* field_y, float* field_y_change, int x, int y) {
	field_y[x + Width * y] = field_y_change[x + Width * y];
}

void compute_frame(float* field_y, float* field_y_prev, float* field_y_change, bool* field_is_wall) {
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

void openCl_compute(float* &field_y, float* &field_y_prev, float* &field_y_change, bool* &field_is_wall,
	std::map<int, cl_mem>& buffers, cl_kernel &kernel_compute, cl_kernel &kernel_apply, cl_command_queue &command_queue) {
	cl_int ret;
	size_t global_work_size[2] = { divider, divider };

#pragma region Compute_changings

	ret = clEnqueueWriteBuffer(command_queue, buffers[0], CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[1], CL_TRUE, 0, Width * Height * sizeof(float), field_y_prev, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[2], CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[3], CL_TRUE, 0, Width * Height * sizeof(bool), field_is_wall, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[4], CL_TRUE, 0, sizeof(cl_int), &x_cell_size, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[5], CL_TRUE, 0, sizeof(cl_int), &y_cell_size, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[6], CL_TRUE, 0, sizeof(float), &r, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[7], CL_TRUE, 0, sizeof(float), &fric_coef, 0, NULL, NULL);

	ret = clSetKernelArg(kernel_compute, 0, sizeof(cl_mem), (void*)&buffers[0]);
	ret = clSetKernelArg(kernel_compute, 1, sizeof(cl_mem), (void*)&buffers[1]);
	ret = clSetKernelArg(kernel_compute, 2, sizeof(cl_mem), (void*)&buffers[2]);
	ret = clSetKernelArg(kernel_compute, 3, sizeof(cl_mem), (void*)&buffers[3]);
	ret = clSetKernelArg(kernel_compute, 4, sizeof(cl_mem), (void*)&buffers[4]);
	ret = clSetKernelArg(kernel_compute, 5, sizeof(cl_mem), (void*)&buffers[5]);
	ret = clSetKernelArg(kernel_compute, 6, sizeof(cl_mem), (void*)&buffers[6]);
	ret = clSetKernelArg(kernel_compute, 7, sizeof(cl_mem), (void*)&buffers[7]);


	ret = clEnqueueNDRangeKernel(command_queue, kernel_compute, 2, NULL, global_work_size, NULL, 0, NULL, NULL); ///////////////


	ret = clEnqueueReadBuffer(command_queue, buffers[0], CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	ret = clEnqueueReadBuffer(command_queue, buffers[1], CL_TRUE, 0, Width * Height * sizeof(float), field_y_prev, 0, NULL, NULL);
	ret = clEnqueueReadBuffer(command_queue, buffers[2], CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
	ret = clEnqueueReadBuffer(command_queue, buffers[3], CL_TRUE, 0, Width * Height * sizeof(bool), field_is_wall, 0, NULL, NULL);
#pragma endregion

#pragma region Apply_changings

	ret = clEnqueueWriteBuffer(command_queue, buffers[0], CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[2], CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[4], CL_TRUE, 0, sizeof(int), &x_cell_size, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, buffers[5], CL_TRUE, 0, sizeof(int), &y_cell_size, 0, NULL, NULL);

	ret = clSetKernelArg(kernel_apply, 0, sizeof(cl_mem), (void*)&buffers[0]);
	ret = clSetKernelArg(kernel_apply, 1, sizeof(cl_mem), (void*)&buffers[2]);
	ret = clSetKernelArg(kernel_apply, 2, sizeof(cl_mem), (void*)&buffers[4]);
	ret = clSetKernelArg(kernel_apply, 3, sizeof(cl_mem), (void*)&buffers[5]);

	ret = clEnqueueNDRangeKernel(command_queue, kernel_apply, 2, NULL, global_work_size, NULL, 0, NULL, NULL); ///////////////


	ret = clEnqueueReadBuffer(command_queue, buffers[0], CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	ret = clEnqueueReadBuffer(command_queue, buffers[2], CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);

#pragma endregion

}

