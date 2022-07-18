__kernel void compute(__global float* field_y, __global float* field_y_prev, __global float* field_y_change,
	__global bool* field_is_wall, __global int* x_cell_size, __global int* y_cell_size)
{
	const int Width = 300;
	const int xid = get_global_id(0); //from 0 to 9
	const int yid = get_global_id(1);
	//const int yid = 0;
	const int x_cell = *x_cell_size;
	const int y_cell = *y_cell_size;

	for (int x = xid * x_cell; x < (xid + 1) * x_cell; x++) {
		for (int y = yid * y_cell; y < (yid + 1) * y_cell; y++) {
			//if (field_is_wall[x + Width * y] == false) {
			//	field_y_change[x + Width * y] = 2 * field_y[x + Width * y] - field_y_prev[x + Width * y] +   ///взаимодействие клетки "квадратом"
			//		r * (field_y[(x)+Width * (y - 1)] + field_y[(x)+Width * (y + 1)] + field_y[(x - 1) + Width * (y)] + field_y[(x + 1) + Width * (y)] - 4 * field_y[(x)+Width * (y)]);
			//
			//	field_y_prev[x + Width * y] = field_y[x + Width * y];
			//}

		}
	}

}