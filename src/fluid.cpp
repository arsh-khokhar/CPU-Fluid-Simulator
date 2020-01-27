#include "fluid.h"

FluidCell::FluidCell(float arg_diffusion, float arg_viscocity, float arg_dt)
{
	size = SIZE;
	dt = arg_dt;
	diffusion = arg_diffusion;
	viscocity = arg_viscocity;
	for (int i = 0; i < SIZE * SIZE; i++)
	{
		velocityX[i] = velocityX_prev[i] = 0.0f;
		density[i] = density_prev[i] = 0.0f;
		velocityY[i] = velocityY_prev[i] = 0.0f;
	}
}

FluidCell::~FluidCell()
{
	delete[] velocityX, velocityX_prev, velocityY, velocityY_prev, density, density_prev;
}