// Copyright 2021 <Zach Whitehead>
// OpenPPG

#include "../../inc/VoltageCurves.h"
#include "../../inc/sp140/shared-config.h"

// simple set of data points from load testing
// maps voltage to battery percentage
float getBatteryPercent(float voltage) {
  float battPercent = 0;

  if (voltage > 94.8) {
    battPercent = mapd(voltage, 94.8, 99.6, 90, 100);
  } else if (voltage > 93.36) {
    battPercent = mapd(voltage, 93.36, 94.8, 80, 90);
  } else if (voltage > 91.68) {
    battPercent = mapd(voltage, 91.68, 93.36, 70, 80);
  } else if (voltage > 89.76) {
    battPercent = mapd(voltage, 89.76, 91.68, 60, 70);
  } else if (voltage > 87.6) {
    battPercent = mapd(voltage, 87.6, 89.76, 50, 60);
  } else if (voltage > 85.2) {
    battPercent = mapd(voltage, 85.2, 87.6, 40, 50);
  } else if (voltage > 82.32) {
    battPercent = mapd(voltage, 82.32, 85.2, 30, 40);
  } else if (voltage > 80.16) {
    battPercent = mapd(voltage, 80.16, 82.32, 20, 30);
  } else if (voltage > 78) {
    battPercent = mapd(voltage, 78, 80.16, 10, 20);
  } else if (voltage > 60.96) {
    battPercent = mapd(voltage, 60.96, 78, 0, 10);
  }
  return constrain(battPercent, 0, 100);
}

unsigned int CELLS_IN_SERIES = 24;
unsigned int CELLS_IN_PARALLEL = 10;

// estimate remaining battery percent using cell manufacturer's voltage curves
float getBatteryPercent(float voltage, float current, float temperature) {
  // calculate cell voltage and current (assume evenly distributed)
  voltage = voltage / CELLS_IN_SERIES;
  current = current / CELLS_IN_PARALLEL;

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
  const VoltageCurve *end = VOLTAGE_CURVES + numCurves;
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


// given a voltage, voltage curve, energy lookup, and length of curve, return the energy (Wh)
float getEnergy(float voltage, const float *voltageCurve, const float *energyLookup, const int len) {
  // find the closest voltage in the curve (voltages are in descending order)
  const float *higherVoltage = voltageCurve;
  const float *lowerVoltage = voltageCurve + 1;
  const float *higherEnergy = energyLookup;
  const float *lowerEnergy = energyLookup + 1;
  const float *end = voltageCurve + len;
  while (lowerVoltage != end && voltage < *lowerVoltage) {
    higherVoltage++;
    lowerVoltage++;
    higherEnergy++;
    lowerEnergy++;
  }
  // remove remaining energy in between both data points
  return mapd(voltage, *higherVoltage, *lowerVoltage, *higherEnergy, *lowerEnergy);
}


// inspired by https://github.com/rlogiacco/BatterySense/
// https://www.desmos.com/calculator/7m9lu26vpy
uint8_t battery_sigmoidal(float voltage, uint16_t minVoltage, uint16_t maxVoltage) {
  uint8_t result = 105 - (105 / (1 + pow(1.724 * (voltage - minVoltage)/(maxVoltage - minVoltage), 5.5)));
  return result >= 100 ? 100 : result;
}
