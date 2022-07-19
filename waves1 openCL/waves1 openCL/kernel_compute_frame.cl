#define Width 500
#define Height 500
#define Screen_Scale 1
#define Screen_Width 500 //Width * Screen_Scale
#define Screen_Height 500 //Height * Screen_Scale

__kernel void compute(__global float* field_y, __global float* field_y_prev, __global float* field_y_change,
	__global bool* field_is_wall, __global int* x_cell_size, __global int* y_cell_size, __global float* r_raw, __global float* fric_raw)
{
	const int xid = get_global_id(0); 
	const int yid = get_global_id(1);
	const int x_cell = *x_cell_size;
	const int y_cell = *y_cell_size;

	const float r = *r_raw;
	const float fric = *fric_raw;

	for (int x = xid * x_cell; x < (xid + 1) * x_cell; x++) {
		for (int y = yid * y_cell; y < (yid + 1) * y_cell; y++) {
			if (field_is_wall[x + Width * y] == false) {
				field_y_change[x + Width * y] = 2 * field_y[x + Width * y] - field_y_prev[x + Width * y] +   ///взаимодействие клетки "квадратом"
					r * (field_y[(x)+Width * (y - 1)] + field_y[(x)+Width * (y + 1)] + field_y[(x - 1) + Width * (y)] + field_y[(x + 1) + Width * (y)] - 4 * field_y[(x)+Width * (y)]);
			
				field_y_prev[x + Width * y] = field_y[x + Width * y];
				field_y_change[x + Width * y] -= (field_y_change[x + Width * y] - field_y[x + Width * y]) * fric;

				if (field_y[x + Width * y] > 50 || field_y[x + Width * y] < -50 || isnan(field_y[x + Width * y])) {
					field_y_change[x + Width * y] = 0;
					field_y[x + Width * y] = 0;
					field_y_prev[x + Width * y] = 0;
				}
			}

		}
	}

}


__kernel void apply(__global float* field_y, __global float* field_y_change, __global int* x_cell_size, __global int* y_cell_size) {
	const int xid = get_global_id(0);
	const int yid = get_global_id(1);
	const int x_cell = *x_cell_size;
	const int y_cell = *y_cell_size;

	for (int x = xid * x_cell; x < (xid + 1) * x_cell; x++) {
		for (int y = yid * y_cell; y < (yid + 1) * y_cell; y++) {
			field_y[x + Width * y] = field_y_change[x + Width * y];
		}
	}
}

__kernel void draw(__global float* field_y, __global bool* field_is_wall, __global unsigned char* pixels, __global int* x_cell_size, __global int* y_cell_size) {
	const int xid = get_global_id(0);
	const int yid = get_global_id(1);
	const int x_cell = *x_cell_size;
	const int y_cell = *y_cell_size;
	
	for (int x = xid * x_cell; x < (xid + 1) * x_cell; x++) {
		for (int y = yid * y_cell; y < (yid + 1) * y_cell; y++) {

			int color, rcolor = 0, bcolor = 0, gcolor = 0;
			float value = field_y[x + Width * y];
			if (field_is_wall[x + Width * y]) {
				rcolor = 255;
				gcolor = 255;
				bcolor = 255;
			}
			else {
				color = value * 100;

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

			if (isnan(value)) {
				gcolor = 250;
				bcolor = 0;
				rcolor = 0;
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

}

