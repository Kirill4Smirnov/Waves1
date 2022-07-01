#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>

#ifndef min
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif // min


using namespace sf;
using std::this_thread::sleep_for;

const int Width = 200;
const int Height = 200;

const int Screen_Scale = 3;
const int Screen_Width = Width * Screen_Scale;
const int Screen_Height = Height * Screen_Scale;


const double r = 0.1;//accuracy of simulation
const double fric_coef = 0.000;

class Point {
private:
	double y_change;
public:
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
	Point *points;
	//Point points[Width][Height];
	//int* data;

	Field() {
		//data = new int[4];
		points = new Point[Width * Height];
	}
	~Field() {};

	void ComputeFrame() {

		for (int i = 1; i < Width - 1; i++) {
			for (int j = 1; j < Height - 1; j++) {
				//points[i][j].Compute(points[i-1][j-1], points[i + 1][j - 1], points[i - 1][j + 1], points[i + 1][j + 1]);
				points[i * Width + j].Compute(points[(i -1 ) * Width + (j - 1)], points[(i + 1) * Width + (j - 1)], points[(i - 1) * Width + (j + 1)], points[(i + 1) * Width + (j + 1)]);
				
			}
		}
		//points[0].Compute(points[1]);
		//points[Width - 1].Compute(points[Width - 2]);

		for (int i = 1; i < Width - 1; i++) {
			for (int j = 1; j < Height - 1; j++) {
				//points[i][j].ApplyChanging();
				points[i * Width + j].ApplyChanging();
			}
		}
	}
};

int main()
{
	Field field;

	for (int x = 0; x < Width; x++) {
		for (int y = 0; y < Height; y++) {
			field.points[x * Width + y].y = 0.0;
			field.points[x * Width + y].y_prev = 0.0;
		}
	}

	for (int x = 20; x < 30; x++) {
		for (int y = 50; y < 70; y++) {
			field.points[x * Width + y].y = 10.0;
			field.points[x * Width + y].y_prev = 11.0;
		}
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
		}

		//coloring of the screen
		for (int x = 0; x < Width; x++) {
			for (int y = 0; y < Height; y++) {
				int color, rcolor = 0, bcolor = 0, gcolor = 0;
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

				
				for (int i = x * Screen_Scale; i < (x + 1) * Screen_Scale; i++) {
					for (int j = y * Screen_Scale; j < (y + 1) * Screen_Scale; j++) {
						int xScreen = i;
						int yScreen = j * Screen_Scale;
						pixels[(xScreen + yScreen * Width) * 4] = rcolor; //r
						pixels[(xScreen + yScreen * Width) * 4 + 1] = gcolor; //g
						pixels[(xScreen + yScreen * Width ) * 4 + 2] = bcolor; //b
						pixels[(xScreen + yScreen * Width ) * 4 + 3] = 255; //a
					}
				}
				
				//pixels[(x + y * Width) * 4] = rcolor; //r
				//pixels[(x + y * Width) * 4 + 1] = gcolor; //g
				//pixels[(x + y * Width) * 4 + 2] = bcolor; //b
				//pixels[(x + y * Width) * 4 + 3] = 255; //a
				
				//pixels[(x * Screen_Scale + y * Screen_Scale * Screen_Width) * 4] = rcolor; //r
				//pixels[(x * Screen_Scale + y * Screen_Scale * Screen_Width) * 4 + 1] = gcolor; //g
				//pixels[(x * Screen_Scale + y * Screen_Scale * Screen_Width) * 4 + 2] = bcolor; //b
				//pixels[(x * Screen_Scale + y * Screen_Scale * Screen_Width) * 4 + 3] = 255; //a

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

		//sleep_for(std::chrono::microseconds((long long) Seconds_per_frame * 1000000));
		sleep_for(std::chrono::milliseconds(10));
		field.ComputeFrame();
	}

	delete[] pixels;
	return 0;
}

