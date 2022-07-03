#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>
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
	Point* points;

	Field() {
		points = new Point[Width * Height];

		for (int x = 0; x < Width; x++) {
			for (int y = 0; y < Height; y++) {
				points[x * Width + y].y = 0.0;
				points[x * Width + y].y_prev = 0.0;
			}
		}

		for (int x = 0; x < Width; x++) {
			points[x * Width + 0].is_wall = true;
			points[x * Width + Height-1].is_wall = true;
		}
		for (int y = 0; y < Height; y++) {
			points[0 * Width + y].is_wall = true;
			points[(Width-1) * Width + y].is_wall = true;
		}
	}
	~Field() {
		delete[] points;
	};

	void ComputeFrame() {

		for (int i = 1; i < Width - 1; i++) {
			for (int j = 1; j < Height - 1; j++) {
				if (!points[i * Width + j].is_wall) {
					points[i * Width + j].Compute(points[(i - 1) * Width + (j - 1)], points[(i + 1) * Width + (j - 1)], points[(i - 1) * Width + (j + 1)], points[(i + 1) * Width + (j + 1)]);
				}

			}
		}

		for (int i = 1; i < Width - 1; i++) {
			for (int j = 1; j < Height - 1; j++) {
				if (!points[i * Width + j].is_wall) {
					points[i * Width + j].ApplyChanging();
				}
			}
		}
	}
};

int main()
{
	Field field;

	/*
	for (int x = 20; x < 30; x++) {
		for (int y = 50; y < 70; y++) {
			field.points[x * Width + y].y = 10.0;
			field.points[x * Width + y].y_prev = 11.0;
		}
	}*/

	for (int x = 50; x < 200; x++) {
		field.points[x * Width + 100].is_wall = true;
		field.points[x * Width + 50].is_wall = true;
	}

	RenderWindow window(VideoMode(Screen_Width, Screen_Height), "Wave simulation");
	window.setFramerateLimit(60);

	Uint8* pixels = new Uint8[4 * Screen_Width * Screen_Width];

	while (window.isOpen())
	{
		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed)
				window.close();
			//if (event.type == Event::MouseMoved) {
			//	std::cout << "x: " << Mouse::getPosition(window).x << "\ty: " << Mouse::getPosition(window).y << '\n';
			//}

			if (Mouse::isButtonPressed(Mouse::Left))
			{
				int x = Mouse::getPosition(window).x;
				int y = Mouse::getPosition(window).y;

				x = x * Width / window.getSize().x;
				y = y * Height / window.getSize().y;


				std::cout << "Left mouse pressed\tx: " << x << "\ty: " << y << '\n';
				if ((x > 3 && x < Width - 3) && (y > 3 && y < Height - 3)) {
					
					for (int i = x - 3; i < x + 3; i++) {
						for (int j = y - 3; j < y + 3; j++) {
							if (!field.points[i * Width + j].is_wall) {
								field.points[i * Width + j] = 3.0;
							}
							
						}
					}
					
				}
			}
		}

		//coloring of the screen
		for (int x = 0; x < Width; x++) {
			for (int y = 0; y < Height; y++) {
				int color, rcolor = 0, bcolor = 0, gcolor = 0;
				if (field.points[x * Width + y].is_wall) {
					rcolor = 255;
					gcolor = 255;
					bcolor = 255;
				}
				else {
					color = field.points[x * Width + y].y * 100;

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

		Image img;
		img.create(Screen_Width, Screen_Height, pixels);
		Texture texture;
		texture.loadFromImage(img);
		Sprite sprite;
		sprite.setTexture(texture, true);
		window.draw(sprite);

		window.display();

		sleep_for(std::chrono::milliseconds(10));
		field.ComputeFrame();
	}

	delete[] pixels;
	return 0;
}
