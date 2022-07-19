#include "Header.h"
#include <chrono>
#include <thread>

#include <iostream>
#include <map>


#define DEBUG

using namespace sf;
using std::this_thread::sleep_for;

//const int Width = 500;
//const int Height = 500;

const int divider = 100; //Width and Height must be divisible by divider

const cl_int x_cell_size = Width / divider;
const cl_int y_cell_size = Height / divider;

//const int Screen_Scale = 1;
//const int Screen_Width = Width * Screen_Scale;
//const int Screen_Height = Height * Screen_Scale;


const float r = 0.1;//accuracy of simulation
const float fric_coef = 0.005;


void openCl_compute(float*& field_y, float*& field_y_prev, float*& field_y_change, bool*& field_is_wall, cl_uchar*& pixels,
	std::map<int, cl_mem>& buffers, cl_kernel& kernel_compute, cl_kernel& kernel_apply, cl_kernel& kernel_draw, cl_command_queue& command_queue);


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
	cl_kernel kernel_compute = NULL, kernel_apply = NULL, kernel_draw = NULL;

	FILE* fp;
	//const char fileNameDefines[] = "Defines.h";
	const char fileNameKernel[] = "kernel_compute_frame.cl";
	size_t source_size;
	char* source_str;

	try {
		fopen_s(&fp, fileNameKernel, "r");
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
	//std::cout << "ret: " << ret << '\n';

	kernel_compute = clCreateKernel(program, "compute", &ret);
	//std::cout << "ret: " << ret << '\n';
	kernel_apply = clCreateKernel(program, "apply", &ret);
	kernel_draw = clCreateKernel(program, "draw", &ret);


	free(source_str);

	cl_mem field_y_to_kernel, field_y_prev_to_kernel, field_y_change_to_kernel, field_is_wall_to_kernel;
	cl_mem x_cell_size_kernel, y_cell_size_kernel, r_kernel, fric_kernel, pixels_kernel;

	size_t global_work_size[2] = { divider, divider };

	field_y_to_kernel = clCreateBuffer(context, CL_MEM_READ_WRITE, Width * Height * sizeof(float), NULL, &ret);
	field_y_prev_to_kernel = clCreateBuffer(context, CL_MEM_READ_WRITE, Width * Height * sizeof(float), NULL, &ret);
	field_y_change_to_kernel = clCreateBuffer(context, CL_MEM_READ_WRITE, Width * Height * sizeof(float), NULL, &ret);
	field_is_wall_to_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, Width * Height * sizeof(bool), NULL, &ret);
	x_cell_size_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_int), NULL, &ret);
	y_cell_size_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_int), NULL, &ret);
	r_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float), NULL, &ret);
	fric_kernel = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float), NULL, &ret);
	pixels_kernel = clCreateBuffer(context, CL_MEM_READ_WRITE, 4 * Screen_Width * Screen_Width * sizeof(cl_uchar), NULL, &ret);


	std::map<int, cl_mem> buffers = {
		{0, field_y_to_kernel},
		{1, field_y_prev_to_kernel},
		{2, field_y_change_to_kernel},
		{3, field_is_wall_to_kernel},
		{4, x_cell_size_kernel},
		{5, y_cell_size_kernel},
		{6, r_kernel},
		{7, fric_kernel},
		{8, pixels_kernel}
	};

#pragma endregion

#pragma region Init_window
	const int offset = 50; //screen offset for text
	RenderWindow window(VideoMode(Screen_Width, Screen_Height + offset), "Wave simulation");
	window.setFramerateLimit(60);

//	Uint8* pixels = new Uint8[4 * Screen_Width * Screen_Width]; ////Uint8 and cl_uchar are the same uchar, but cl_uchar is better to pass into a kernel
	cl_uchar* pixels = new cl_uchar[4 * Screen_Width * Screen_Width];

	short brush_size = 3;
	float value = 3.0;
	bool cursor_enabled = true;

	bool C_flag = false;
	std::string outText = "";
	
	Font font;
	Text text;
	font.loadFromFile("..\\..\\arialmt.ttf");  //your path here
	text.setFont(font);

	text.setCharacterSize(23); // in pixels, not points!
	text.setFillColor(sf::Color::White);
	text.move(10.f, 5.f);
	std::ostringstream oss;

	oss << "Brush size: " << brush_size << "\tBrush value: " << value << "\tDraw mode enabled: " << cursor_enabled;
	outText = oss.str();
	text.setString(outText);

#pragma endregion

#pragma region Animation
	sf::Clock clock; // starts the clock
	
	
	float fps = 0.0;
	int frame_counter = 0;
	//std::cout << '\n';
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
					//if W pressed, add wall
					if (Keyboard::isKeyPressed(Keyboard::W)) { 
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

				//delete several walls
				if (Mouse::isButtonPressed(Mouse::Right)) 
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

#ifdef DEBUG
			///debug
			if (Keyboard::isKeyPressed(Keyboard::D)) {
				if((0 <= x && x < Width) && (0 <= y && y < Height)) std::cout << "x: " << x << "\ty: " << y << "\tvalue: " << field_y[x + y * Width] << '\n';
			}
#endif
		}
		//compute frame and color the screen
		openCl_compute(field_y, field_y_prev, field_y_change, field_is_wall, pixels, buffers, kernel_compute, kernel_apply, kernel_draw, command_queue);

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
		frame_counter++;

		if (frame_counter > 30) {
			sf::Time elapsed4 = clock.getElapsedTime();
			fps = frame_counter * 1000.0 / elapsed4.asMilliseconds();
			clock.restart();
			std::cout << "fps: " << fps << '\r' << std::flush;
			frame_counter = 0;
		}
		
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


void openCl_compute(float* &field_y, float* &field_y_prev, float* &field_y_change, bool* &field_is_wall, cl_uchar* &pixels,
	std::map<int, cl_mem>& buffers, cl_kernel &kernel_compute, cl_kernel &kernel_apply, cl_kernel& kernel_draw, cl_command_queue &command_queue) {
	cl_int retn;
	size_t global_work_size[2] = { divider, divider };

#pragma region Compute_changings

	retn = clEnqueueWriteBuffer(command_queue, buffers[0], CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[1], CL_TRUE, 0, Width * Height * sizeof(float), field_y_prev, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[2], CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[3], CL_TRUE, 0, Width * Height * sizeof(bool), field_is_wall, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[4], CL_TRUE, 0, sizeof(cl_int), &x_cell_size, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[5], CL_TRUE, 0, sizeof(cl_int), &y_cell_size, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[6], CL_TRUE, 0, sizeof(float), &r, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[7], CL_TRUE, 0, sizeof(float), &fric_coef, 0, NULL, NULL);

	retn = clSetKernelArg(kernel_compute, 0, sizeof(cl_mem), (void*)&buffers[0]);
	retn = clSetKernelArg(kernel_compute, 1, sizeof(cl_mem), (void*)&buffers[1]);
	retn = clSetKernelArg(kernel_compute, 2, sizeof(cl_mem), (void*)&buffers[2]);
	retn = clSetKernelArg(kernel_compute, 3, sizeof(cl_mem), (void*)&buffers[3]);
	retn = clSetKernelArg(kernel_compute, 4, sizeof(cl_mem), (void*)&buffers[4]);
	retn = clSetKernelArg(kernel_compute, 5, sizeof(cl_mem), (void*)&buffers[5]);
	retn = clSetKernelArg(kernel_compute, 6, sizeof(cl_mem), (void*)&buffers[6]);
	retn = clSetKernelArg(kernel_compute, 7, sizeof(cl_mem), (void*)&buffers[7]);


	retn = clEnqueueNDRangeKernel(command_queue, kernel_compute, 2, NULL, global_work_size, NULL, 0, NULL, NULL); ///////////////


	retn = clEnqueueReadBuffer(command_queue, buffers[0], CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	retn = clEnqueueReadBuffer(command_queue, buffers[1], CL_TRUE, 0, Width * Height * sizeof(float), field_y_prev, 0, NULL, NULL);
	retn = clEnqueueReadBuffer(command_queue, buffers[2], CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
	retn = clEnqueueReadBuffer(command_queue, buffers[3], CL_TRUE, 0, Width * Height * sizeof(bool), field_is_wall, 0, NULL, NULL);
#pragma endregion

#pragma region Apply_changings

	retn = clEnqueueWriteBuffer(command_queue, buffers[0], CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[2], CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[4], CL_TRUE, 0, sizeof(int), &x_cell_size, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[5], CL_TRUE, 0, sizeof(int), &y_cell_size, 0, NULL, NULL);

	retn = clSetKernelArg(kernel_apply, 0, sizeof(cl_mem), (void*)&buffers[0]);
	retn = clSetKernelArg(kernel_apply, 1, sizeof(cl_mem), (void*)&buffers[2]);
	retn = clSetKernelArg(kernel_apply, 2, sizeof(cl_mem), (void*)&buffers[4]);
	retn = clSetKernelArg(kernel_apply, 3, sizeof(cl_mem), (void*)&buffers[5]);

	retn = clEnqueueNDRangeKernel(command_queue, kernel_apply, 2, NULL, global_work_size, NULL, 0, NULL, NULL); ///////////////


	retn = clEnqueueReadBuffer(command_queue, buffers[0], CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	//retn = clEnqueueReadBuffer(command_queue, buffers[2], CL_TRUE, 0, Width * Height * sizeof(float), field_y_change, 0, NULL, NULL);

#pragma endregion

#pragma region Draw_window

	retn = clEnqueueWriteBuffer(command_queue, buffers[8], CL_TRUE, 0, 4 * Screen_Width * Screen_Width * sizeof(cl_uchar), pixels, 0, NULL, NULL);
	//retn = clEnqueueWriteBuffer(command_queue, buffers[0], CL_TRUE, 0, Width * Height * sizeof(float), field_y, 0, NULL, NULL);
	//retn = clEnqueueWriteBuffer(command_queue, buffers[3], CL_TRUE, 0, Width * Height * sizeof(float), field_is_wall, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[4], CL_TRUE, 0, sizeof(int), &x_cell_size, 0, NULL, NULL);
	retn = clEnqueueWriteBuffer(command_queue, buffers[5], CL_TRUE, 0, sizeof(int), &y_cell_size, 0, NULL, NULL);

	retn = clSetKernelArg(kernel_draw, 0, sizeof(cl_mem), (void*)&buffers[0]);
	retn = clSetKernelArg(kernel_draw, 1, sizeof(cl_mem), (void*)&buffers[3]);
	retn = clSetKernelArg(kernel_draw, 2, sizeof(cl_mem), (void*)&buffers[8]); /////
	retn = clSetKernelArg(kernel_draw, 3, sizeof(cl_mem), (void*)&buffers[4]);
	retn = clSetKernelArg(kernel_draw, 4, sizeof(cl_mem), (void*)&buffers[5]);

	retn = clEnqueueNDRangeKernel(command_queue, kernel_draw, 2, NULL, global_work_size, NULL, 0, NULL, NULL); ///////////////
	//std::cout << "retn: " << retn << '\n';
	

	retn = clEnqueueReadBuffer(command_queue, buffers[8], CL_TRUE, 0, 4 * Screen_Width * Screen_Width * sizeof(cl_uchar), pixels, 0, NULL, NULL);


#pragma endregion

}

