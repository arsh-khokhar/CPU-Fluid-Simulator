#pragma once
#ifndef FLUID_H
#define FLUID_H
#define SIZE 64

class FluidCell
{
public:
	int size;
	float diffusion, viscocity, dt;
	float velocityX[SIZE * SIZE];
	float velocityX_prev[SIZE * SIZE];
	float velocityY[SIZE * SIZE];
	float velocityY_prev[SIZE * SIZE];
	float density[SIZE * SIZE];
	float density_prev[SIZE * SIZE];
	FluidCell(float arg_diffusion, float arg_viscocity, float arg_dt);
	~FluidCell();
};

#endif