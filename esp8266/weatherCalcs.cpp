#include "weatherCalcs.h"
//#include <math.h>

float heatIndex(float T, float R){
  float c1,c2,c3,c4,c5,c6,c7,c8,c9;


  float HI = 0.5 * (T + 61.0 + ((T-68.0)*1.2) + (R*0.094));

  if (HI > 80.0){
    c1 = -42.379;
    c2=2.04901523;
    c3=10.144333127;
    c4=-0.22475541;
    c5=-6.83783e-3;
    c6=-5.481717e-2;
    c7=1.22874e-3;
    c8=8.5282e-4;
    c9=-1.99e-6;
    float T2 = T*T;
    float R2 = R*R;
    HI = c1 + c2 * T + c3*R + c4 * T * R + c5 * T2 +
            c6 * R2 + c7 * T2 * R + c8 * T * R2 + c9 * T2 * R2;
  }


  return HI;
}


float dewPoint(float farenheight, float humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
        float celsius = (farenheight - 32.0)/1.8;
  float RATIO = 373.15 / (273.15 + celsius);
  float RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  float VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
  float T = log(VP/0.61078);   // temp var
  return (241.88 * T) / (17.558 - T) * 1.8 + 32.0;
}

float meanSeaLevelPressure(float P, float T, float h){
	float TK = (T - 32.)* 5./9. + 273.15;
	float r1 = 1. - 0.0065 * h / ( TK + 0.0065 * h);
	float r2 = P * pow(r1,-5.257);	// P inHG to hPa

	return r2;
}



const int Ndirections=8;
float voltFracByDir[Ndirections]={
    0.77, 0.46, 0.09, 0.18, 0.29, 0.62, 0.92, 0.87};
float windDirArray[Ndirections]={
  0, 45, 90, 135, 180, 225, 270, 315};



float windDirByVoltageFraction( float inputVoltageFraction)
{
  float Vdiffmin=1.0;
  float windDir;
  for (int i=0; i<Ndirections; i++){
    float Vdiff = fabs(inputVoltageFraction - voltFracByDir[i]);
    if (Vdiffmin > Vdiff){
      Vdiffmin = Vdiff;
      windDir = windDirArray[i];
    }
  }
  return windDir;

}
