#include "simulator.h"
#include "fluid.h"
#include <glm/glm.hpp>

FluidSimulator::FluidSimulator(FluidCell* arg_fluidCell, int arg_numIterations)
{
	FLUID_CELL = arg_fluidCell;
	GRID_SIZE = arg_fluidCell->size;
	NUM_ITERATIONS = arg_numIterations;
}

FluidSimulator::~FluidSimulator()
{
	if (!FLUID_CELL)
	{
		free(FLUID_CELL);
	}
}

int FluidSimulator::GenerateIndex(int arg_x, int arg_y)
{
	return arg_x + arg_y * GRID_SIZE;
}

void FluidSimulator::addDye(int arg_posX, int arg_posY, float arg_amount)
{
	int index = GenerateIndex(arg_posX, arg_posY);
	FLUID_CELL->density[index] += arg_amount;
}

void FluidSimulator::addVelocity(int arg_posX, int arg_posY, float arg_amountX, float arg_amountY)
{
	int index = GenerateIndex(arg_posX, arg_posY);
	FLUID_CELL->velocityX[index] += arg_amountX;
	FLUID_CELL->velocityY[index] += arg_amountY;
}

void FluidSimulator::diffuse(int b, float* arg_velocities, float* arg_velocities_prev, float arg_diff, float arg_dt)
{
	float a = arg_dt * arg_diff * (GRID_SIZE - 2) * (GRID_SIZE - 2);
	linearSolve(b, arg_velocities, arg_velocities_prev, a, 1.0f + 6.0f * a);
}

void FluidSimulator::linearSolve(int b, float* arg_velocities, float* arg_velocities_prev, float a, float c)
{
	float cInverse = 1.0f / c;
	for (int k = 0; k < NUM_ITERATIONS; k++) {
		for (int j = 1; j < GRID_SIZE - 1; j++) {
			for (int i = 1; i < GRID_SIZE - 1; i++) {
				arg_velocities[GenerateIndex(i, j)] = (arg_velocities_prev[GenerateIndex(i, j)]
					+ a * (arg_velocities[GenerateIndex(i + 1, j)]
						+ arg_velocities[GenerateIndex(i - 1, j)]
						+ arg_velocities[GenerateIndex(i, j + 1)]
						+ arg_velocities[GenerateIndex(i, j - 1)]
						)) * cInverse;
			}
		}
		setBoundaries(b, arg_velocities);
	}
}

void FluidSimulator::project(float* arg_veloX, float* arg_veloY, float* p, float* div)
{
	for (int j = 1; j < GRID_SIZE - 1; j++) {
		for (int i = 1; i < GRID_SIZE - 1; i++) {
			div[GenerateIndex(i, j)] = -0.5f * (
				arg_veloX[GenerateIndex(i + 1, j)]
				- arg_veloX[GenerateIndex(i - 1, j)]
				+ arg_veloY[GenerateIndex(i, j + 1)]
				- arg_veloY[GenerateIndex(i, j - 1)]
				) / GRID_SIZE;
			p[GenerateIndex(i, j)] = 0;
		}
	}
	setBoundaries(0, div);
	setBoundaries(0, p);
	linearSolve(0, p, div, 1, 6);

	for (int j = 1; j < GRID_SIZE - 1; j++) {
		for (int i = 1; i < GRID_SIZE - 1; i++) {
			arg_veloX[GenerateIndex(i, j)] -= 0.5f * (p[GenerateIndex(i + 1, j)] - p[GenerateIndex(i - 1, j)]) * GRID_SIZE;
			arg_veloY[GenerateIndex(i, j)] -= 0.5f * (p[GenerateIndex(i, j + 1)] - p[GenerateIndex(i, j - 1)]) * GRID_SIZE;
		}
	}
	setBoundaries(1, arg_veloX);
	setBoundaries(2, arg_veloY);
}

void FluidSimulator::advect(int b, float* arg_dyeVal, float* arg_dyeValPrev, float* arg_veloX, float* arg_veloY, float dt)
{
	float i0, i1, j0, j1;

	float dtx = dt * (GRID_SIZE - 2);
	float dty = dt * (GRID_SIZE - 2);

	float s0, s1, t0, t1;
	float tmp1, tmp2, x, y;

	float arg_gridSizefloat = (float)GRID_SIZE;
	float ifloat, jfloat;
	int i, j;

	for (j = 1, jfloat = 1; j < GRID_SIZE - 1; j++, jfloat++) {
		for (i = 1, ifloat = 1; i < GRID_SIZE - 1; i++, ifloat++) {
			tmp1 = dtx * arg_veloX[GenerateIndex(i, j)];
			tmp2 = dty * arg_veloY[GenerateIndex(i, j)];
			x = ifloat - tmp1;
			y = jfloat - tmp2;
			if (x < 0.5f) x = 0.5f;
			if (x > arg_gridSizefloat + 0.5f) x = arg_gridSizefloat + 0.5f;
			i0 = floor(x);
			i1 = i0 + 1.0f;
			if (y < 0.5f) y = 0.5f;
			if (y > arg_gridSizefloat + 0.5f) y = arg_gridSizefloat + 0.5f;
			j0 = floor(y);
			j1 = j0 + 1.0f;
			s1 = x - i0;
			s0 = 1.0f - s1;
			t1 = y - j0;
			t0 = 1.0f - t1;

			int i0i = (int)i0;
			int i1i = (int)i1;
			int j0i = (int)j0;
			int j1i = (int)j1;

			arg_dyeVal[GenerateIndex(i, j)] =
				s0 * (t0 * arg_dyeValPrev[GenerateIndex(i0i, j0i)] + t1 * arg_dyeValPrev[GenerateIndex(i0i, j1i)]) +
				s1 * (t0 * arg_dyeValPrev[GenerateIndex(i1i, j0i)] + t1 * arg_dyeValPrev[GenerateIndex(i1i, j1i)]);
		}
	}
	setBoundaries(b, arg_dyeVal);
}

void FluidSimulator::setBoundaries(int b, float* x)
{
	for (int i = 1; i < GRID_SIZE - 1; i++) {
		x[GenerateIndex(i, 0)] = b == 2 ? -x[GenerateIndex(i, 1)] : x[GenerateIndex(i, 1)];
		x[GenerateIndex(i, GRID_SIZE - 1)] = b == 2 ? -x[GenerateIndex(i, GRID_SIZE - 2)] : x[GenerateIndex(i, GRID_SIZE - 2)];
	}

	for (int j = 1; j < GRID_SIZE - 1; j++) {
		x[GenerateIndex(0, j)] = b == 1 ? -x[GenerateIndex(1, j)] : x[GenerateIndex(1, j)];
		x[GenerateIndex(GRID_SIZE - 1, j)] = b == 1 ? -x[GenerateIndex(GRID_SIZE - 2, j)] : x[GenerateIndex(GRID_SIZE - 2, j)];
	}

	x[GenerateIndex(0, 0)] = (x[GenerateIndex(1, 0)] + x[GenerateIndex(0, 1)]) * 0.5f;
	x[GenerateIndex(0, GRID_SIZE - 1)] = (x[GenerateIndex(1, GRID_SIZE - 1)] + x[GenerateIndex(0, GRID_SIZE - 2)]) * 0.5f;
	x[GenerateIndex(GRID_SIZE - 1, 0)] = (x[GenerateIndex(GRID_SIZE - 2, 0)] + x[GenerateIndex(GRID_SIZE - 1, 1)]) * 0.5f;
	x[GenerateIndex(GRID_SIZE - 1, GRID_SIZE - 1)] = (x[GenerateIndex(GRID_SIZE - 2, GRID_SIZE - 1)] + x[GenerateIndex(GRID_SIZE - 1, GRID_SIZE - 2)]) * 0.5f;
}

void FluidSimulator::step()
{
	float visc = FLUID_CELL->viscocity;
	float diff = FLUID_CELL->diffusion;
	float dt = FLUID_CELL->dt;
	float* vx0 = FLUID_CELL->velocityX_prev;
	float* vx = FLUID_CELL->velocityX;
	float* vy0 = FLUID_CELL->velocityY_prev;
	float* vy = FLUID_CELL->velocityY;
	float* densityPrev = FLUID_CELL->density_prev;
	float* density = FLUID_CELL->density;

	diffuse(1, vx0, vx, visc, dt);
	diffuse(2, vy0, vy, visc, dt);

	project(vx0, vy0, vx, vy);

	advect(1, vx, vx0, vx0, vy0, dt);
	advect(2, vy, vy0, vx0, vy0, dt);

	project(vx, vy, vx0, vy0);

	diffuse(0, densityPrev, density, diff, dt);
	advect(0, density, densityPrev, vx, vy, dt);
}