#pragma once

struct TransformSmoothParameters
{
	float	MaxDeviationRadius;
	float	Smoothing;
	float	Correction;
	int		Prediction;
	float	JitterRadius;
};
