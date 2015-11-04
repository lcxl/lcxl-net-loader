
/*
author:
LCX
abstract:
kalman滤波头文件
*/
#ifndef _KALMAN_H_
#define _KALMAN_H_


typedef struct _KALMAN_STATE {
	double prevData;
	double p;
	double q;
	double r;
	double kGain;
} KALMAN_STATE, *PKALMAN_STATE;//保存kalman中间状态

__inline void InitKalmanState(PKALMAN_STATE state) {
	state->prevData = 0;
	state->p = 10;
	state->q = 0.0001;
	state->r = 0.05;
	state->kGain = 0;
}

double KalmanFilter(double inData, PKALMAN_STATE state);

#endif // !_KALMAN_H_
