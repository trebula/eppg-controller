#include <iostream>
#include <string>
#include <random>
using namespace std;

const float exactCapacityWh = 15.5;
const float wattsHoursUsed = 0.5;

const float BATTERY_ANALYSIS_START = 1000;
const float BATTERY_ANALYSIS_END = 30000;
float initialSOCMovingAvg = 0;
int numInitialSOCReadings = 0;
float batteryPercent = 50;

random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0, 100);

float millis() {
    return 0;
}

float getBatteryPercent(float voltage) {
    // random number generator
    return dis(gen);
}

float getBatteryVoltSmoothed() {
    return 0;
}

float constrain(float value, float min, float max) {
  if (value < min) {
    return min;
  }
  else if (value > max) {
    return max;
  }
    return value;
}

// get initial battery percent and update moving average
float analyzeInitialBatteryPercent() {
  // use voltage curve to estimate SOC
  float avgVoltage = getBatteryVoltSmoothed();
  float batteryPercent = getBatteryPercent(avgVoltage);

  if (millis() > BATTERY_ANALYSIS_START) {
    initialSOCMovingAvg = (initialSOCMovingAvg * numInitialSOCReadings + batteryPercent) / (numInitialSOCReadings + 1);
    numInitialSOCReadings++;
    return initialSOCMovingAvg;
  }
  return batteryPercent;
}

void updateBatteryPercent() {
  if (millis() < BATTERY_ANALYSIS_END) {
    batteryPercent = analyzeInitialBatteryPercent();
  }
  else {
    float currentEnergy = batteryPercent / 100 * exactCapacityWh;
    float newEnergy = currentEnergy - wattsHoursUsed;
    batteryPercent = newEnergy / exactCapacityWh * 100;
  }
  batteryPercent = constrain(batteryPercent, 0, 100);
}

int main() {
  for (int i = 0; i < 100; i++) {
    updateBatteryPercent();
    cout << "batteryPercent: " << batteryPercent << endl;
  }
  return 0;
}
