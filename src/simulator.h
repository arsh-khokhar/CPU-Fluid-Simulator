#pragma once
#include "fluid.h"
#ifndef SIMULATOR_H
#define SIMULATOR_H

class FluidSimulator
{
public:
	int GRID_SIZE;
	int NUM_ITERATIONS;
	FluidCell* FLUID_CELL;
	FluidSimulator(FluidCell* arg_fluidCell, int arg_numIterations);
	~FluidSimulator();
	int GenerateIndex(int arg_x, int arg_y);
	void addDye(int arg_posX, int arg_posY, float arg_amount);
	void addVelocity(int arg_posX, int arg_posY, float arg_amountX, float arg_amountY);
	void diffuse(int b, float* arg_velocities, float* arg_velocities_prev, float arg_diff, float arg_dt);
	void linearSolve(int b, float* arg_velocities, float* arg_velocities_prev, float a, float c);
	void project(float* arg_veloX, float* arg_veloY, float* p, float* div);
	void advect(int b, float* arg_dyeVal, float* arg_dyeValPrev, float* arg_veloX, float* arg_veloY, float dt);
	void setBoundaries(int b, float* x);
	void step();
};

#endif