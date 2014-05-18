#include "kalman.h"

double KalmanFilter(double inData, PKALMAN_STATE state)
{

	//Kalman filter function start*******************************
	state->p = state->p + state->q;
	state->kGain = state->p / (state->p + state->r);

	inData = state->prevData + (state->kGain*(inData - state->prevData));

	state->p = (1 - state->kGain)*state->p;

	state->prevData = inData;
	//Kalman filter function stop********************************

	return inData;
}