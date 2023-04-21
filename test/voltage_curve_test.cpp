#include "VoltageCurves.h"
#include <iostream>
#include <random>
using namespace std;

// v1 ESC telemetry
typedef struct {
  float volts;
  float temperatureC;
  float amps;
  float eRPM;
  float inPWM;
  float outPWM;
}STR_ESC_TELEMETRY_140;

// random number generator
random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0, 100);

static STR_ESC_TELEMETRY_140 telemetryData;
float ambientTempC = 0;
int cellsInSeries = 24;
int cellsInParallel = 10;
float batteryPercent = 0;

float constrain(float value, float min, float max) {
  if (value < min) {
    return min;
  }
  else if (value > max) {
    return max;
  }
    return value;
}

// Map double values
double mapd(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// given a voltage, voltage curve, energy lookup, and length of curve, return the energy (Wh)
float getEnergy(float voltage, const float *voltageCurve, const float *energyLookup, const int len) {
  // find the closest voltage in the curve (voltages are in descending order)
  const float *higherVoltage = voltageCurve;
  const float *lowerVoltage = voltageCurve + 1;
  const float *higherEnergy = energyLookup;
  const float *lowerEnergy = energyLookup + 1;
  const float *end = voltageCurve + len - 1;
  while (lowerVoltage != end && voltage < *lowerVoltage) {
    higherVoltage++;
    lowerVoltage++;
    higherEnergy++;
    lowerEnergy++;
  }
  // remove remaining energy in between both data points
  return mapd(voltage, *higherVoltage, *lowerVoltage, *higherEnergy, *lowerEnergy);
}

// estimate remaining battery percent using cell manufacturer's voltage curves
float getBatteryPercent(float voltage) {
  float current = telemetryData.amps;
  const float temperature = ambientTempC;
  // calculate cell voltage and current (assume evenly distributed)
  voltage = voltage / cellsInSeries;
  current = current / cellsInParallel;

  // voltage curves measured at 23C
  // temperature compensation at cold temperatures
  if (temperature < 0) {
    // 8.4% voltage drop from 23C to -20C
    voltage = voltage * mapd(temperature, 23, -20, 1, 1.084);
  }
  else if (temperature < 10) {
    // 3.5% voltage drop from 23C to 0C
    voltage = voltage * mapd(temperature, 23, 0, 1, 1.035);
  }

  // find the two voltage curves with the closest current
  const int numCurves = sizeof(VOLTAGE_CURVES) / sizeof(*VOLTAGE_CURVES);
  const VoltageCurve *lower = VOLTAGE_CURVES;
  const VoltageCurve *higher = VOLTAGE_CURVES + 1;
  const VoltageCurve *end = VOLTAGE_CURVES + numCurves - 1;
  while (higher != end && current > higher->current) {
    lower++;
    higher++;
  }

  // interpolate between the two curves
  float energyLowerCurrent = getEnergy(voltage, lower->voltageCurve, lower->energyLookup, lower->numPoints);
  float energyHigherCurrent = getEnergy(voltage, higher->voltageCurve, higher->energyLookup, higher->numPoints);
  float remainingEnergy = mapd(current, lower->current, higher->current, energyLowerCurrent, energyHigherCurrent);
  float totalEnergy = mapd(current, lower->current, higher->current, lower->totalEnergy, higher->totalEnergy);
  return constrain(remainingEnergy / totalEnergy * 100, 0, 100);
}

int main() {
  telemetryData.volts = 0;
  telemetryData.temperatureC = 0;
  telemetryData.amps = 0;
  telemetryData.eRPM = 0;
  telemetryData.inPWM = 0;
  telemetryData.outPWM = 0;
  ambientTempC = 0;

  // test
  for (int i = 0; i < 100; i++) {
    telemetryData.volts = 90;//dis(gen);
    telemetryData.temperatureC = dis(gen);
    telemetryData.amps = 250;//dis(gen);
    telemetryData.eRPM = dis(gen);
    telemetryData.inPWM = dis(gen);
    telemetryData.outPWM = dis(gen);
    ambientTempC = 20;//dis(gen);
    batteryPercent = getBatteryPercent(telemetryData.volts);
  }
  return 0;
}
