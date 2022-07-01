#include <chrono>
#include <thread>

#include <SFML/Graphics.hpp>

using namespace sf;
using std::this_thread::sleep_for;

const int Width = 100;
const int Height = 100;


const int Screen_Scale = 1;

//r = (self.c * dt/np.gradient(self.x))**2
const double r = 0.01;
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
public:
	Point points[Width][Height];

	Field() {};
	~Field() {};

	void ComputeFrame() {

		for (int i = 1; i < Width - 1; i++) {
			for (int j = 1; j < Height - 1; j++) {
				points[i][j].Compute(points[i-1][j-1], points[i + 1][j - 1], points[i - 1][j + 1], points[i + 1][j + 1]);
			}
		}
		//points[0].Compute(points[1]);
		//points[Width - 1].Compute(points[Width - 2]);

		for (int i = 1; i < Width - 1; i++) {
			for (int j = 1; j < Height - 1; j++) {
				points[i][j].ApplyChanging();
			}
		}
	}
};

int main()
{
	Field field;

	for (int x = 0; x < Width; x++) {
		for (int y = 0; y < Height; y++) {
			field.points[x][y].y = 0.0;
			field.points[x][y].y_prev = 0.0;
		}
	}

	for (int x = 20; x < 30; x++) {
		for (int y = 50; y < 70; y++) {
			field.points[x][y].y = 1.0;
			field.points[x][y].y_prev = 1.0;
		}
	}

	RenderWindow window(VideoMode(Width * Screen_Scale, Height * Screen_Scale), "Wave simulation");
	window.setFramerateLimit(60);


	Uint8* pixels = new Uint8[4 * Width * Height * Screen_Scale * Screen_Scale];

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
				int color = 100;
				
				color = field.points[x][y].y * 10;

				if (color > 255) color = 255;


				pixels[(x + y * Width) * 4] = color; //r
				pixels[(x + y * Width) * 4 + 1] = color; //g
				pixels[(x + y * Width) * 4 + 2] = color; //b
				pixels[(x + y * Width) * 4 + 3] = 255; //a
				

			}
		}

		Image img;
		img.create(Width, Height, pixels);
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

