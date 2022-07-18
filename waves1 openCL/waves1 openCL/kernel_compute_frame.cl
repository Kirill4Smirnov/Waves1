#define Width 300
#define Height 300

__kernel void compute(__global float* field_y, __global float* field_y_prev, __global float* field_y_change,
	__global bool* field_is_wall, __global int* x_cell_size, __global int* y_cell_size, __global float* r_raw, __global float* fric_raw)
{
	const int xid = get_global_id(0); //from 0 to 9
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

				if (field_y[x + Width * y] > 50 || field_y[x + Width * y] < -50) {
					field_y[x + Width * y] = 0;
					field_y_prev[x + Width * y] = 0;
				}
			}

		}
	}

}


__kernel void apply(__global float* field_y, __global float* field_y_change, __global int* x_cell_size, __global int* y_cell_size) {
	const int xid = get_global_id(0); //from 0 to 9
	const int yid = get_global_id(1);
	const int x_cell = *x_cell_size;
	const int y_cell = *y_cell_size;

	for (int x = xid * x_cell; x < (xid + 1) * x_cell; x++) {
		for (int y = yid * y_cell; y < (yid + 1) * y_cell; y++) {
			field_y[x + Width * y] = field_y_change[x + Width * y];
		}
	}
}