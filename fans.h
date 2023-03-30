#include "esp32-hal-ledc.h"
class Temperature {
public:
  int cpu;
  int gpu;
  int ssd;
  Temperature();
  Temperature(int cpuValue, int gpuValue, int ssdValue);
};
Temperature::Temperature() {
  cpu = 0;
  gpu = 0;
  ssd = 0;
}
Temperature::Temperature(int cpuValue, int gpuValue, int ssdValue) {
  cpu = cpuValue;
  gpu = gpuValue;
  ssd = ssdValue;
}

Temperature thermoBuffer[10];  // = Temperature[10];

Relay cpuRelay(CPU_RELAY, true);
Relay gpuRelay(GPU_RELAY, true);
Relay ssdRelay(SSD_RELAY, true);

volatile unsigned long cpuPulses = 0;
volatile unsigned long gpuPulses = 0;
volatile unsigned long ssdPulses = 0;

unsigned long cpuOldPulses = 0;
unsigned long gpuOldPulses = 0;
unsigned long ssdOldPulses = 0;

unsigned long lastCpuRPMmillis = 0;
unsigned long lastGpuRPMmillis = 0;
unsigned long lastSsdRPMmillis = 0;

bool gpuFlag = 0;
bool cpuFlag = 0;
bool ssdFlag = 0;
double cpuCompValue = 0.0;
double gpuCompValue = 0.0;
double ssdCompValue = 0.0;
float compSpeedDiff = 0.0;

void IRAM_ATTR countCpuPulse() {
  cpuPulses++;
}

void IRAM_ATTR countGpuPulse() {
  gpuPulses++;
}

void IRAM_ATTR countSsdPulse() {
  ssdPulses++;
}


// const double cpuMin = 40.0;
// const double cpuMax = 90.0;
// const double gpuMin = 40.0;
// const double gpuMax = 80.0;
// const double ssdMin = 35.0;
// const double ssdMax = 60.0;
// const int pwm_max = 1000;


long calcPwmDuty(double temp, long tempMin, long tempMax, long pwmMin, long pwmMax) {
  double calcTemp = (max(double(tempMin), temp) - tempMin) * 100.0 / (tempMax - tempMin) + SPEED_DIFF;
  calcTemp = max(0.0, min(100.0, calcTemp)) * 100.0;
  return map(int(calcTemp), 0, 10000, pwmMin, pwmMax);
}

// void setCpuPwmDuty(double temp) {
//   OCR3A = calcPwmDuty(temp, cpuMin, cpuMax, 50, pwm_max);
// }
// void setGpuPwmDuty(double temp) {
//   OCR4A = calcPwmDuty(temp, gpuMin, gpuMax, 50, pwm_max);
// }
// void setSsdPwmDuty(double temp) {
//   OCR1A = calcPwmDuty(temp, ssdMin, ssdMax, 100, pwm_max);
// }

unsigned long calculateCpuRPM(unsigned long currentInterrapts, unsigned long currentTime) {
  unsigned long RPM;
  // noInterrupts();
  float elapsedMS = (currentTime - lastCpuRPMmillis) / (1000.0);
  unsigned long revolutions = (currentInterrapts - cpuOldPulses) / 2.0;
  // Serial.print(" revolutions = ");
  // Serial.println(revolutions);
  // Serial.print(" elapsedMS = ");
  // Serial.println(elapsedMS);
  RPM = 60 * revolutions / elapsedMS;
  lastCpuRPMmillis = currentTime;
  cpuOldPulses = currentInterrapts;
  return RPM;
}

unsigned long calculateGpuRPM(unsigned long currentInterrapts, unsigned long currentTime) {
  unsigned long RPM;
  // noInterrupts();
  float elapsedMS = (currentTime - lastGpuRPMmillis) / (1000.0);
  unsigned long revolutions = (currentInterrapts - gpuOldPulses) / 2.0;
  RPM = 60 * revolutions / elapsedMS;
  lastGpuRPMmillis = currentTime;
  gpuOldPulses = currentInterrapts;
  return RPM;
}

unsigned long calculateSsdRPM(unsigned long currentInterrapts, unsigned long currentTime) {
  unsigned long RPM;
  // noInterrupts();
  float elapsedMS = (currentTime - lastSsdRPMmillis) / (1000.0);
  unsigned long revolutions = (currentInterrapts - ssdOldPulses) / 2.0;
  RPM = 60 * revolutions / elapsedMS;
  lastSsdRPMmillis = currentTime;
  ssdOldPulses = currentInterrapts;
  return RPM;
}

void updateThermoBuffer(Temperature newValue) {
  long cpuTemp = newValue.cpu;
  long gpuTemp = newValue.gpu;
  long ssdTemp = newValue.ssd;
  for (int i = 0; i < 9; i++) {
    thermoBuffer[i] = thermoBuffer[i + 1];
    cpuTemp = cpuTemp + thermoBuffer[i].cpu;
    gpuTemp = gpuTemp + thermoBuffer[i].gpu;
    ssdTemp = ssdTemp + thermoBuffer[i].ssd;
  }
  thermoBuffer[9] = newValue;
  cpuCompValue = cpuTemp / 10.0;
  gpuCompValue = gpuTemp / 10.0;
  ssdCompValue = ssdTemp / 10.0;
}

void clearThermoBuffer() {
  for (int i = 0; i < 10; i++) {
    thermoBuffer[i] = Temperature();
  }
  cpuCompValue = 0;
  gpuCompValue = 0;
  ssdCompValue = 0;
}

void setupFans() {
  ledcSetup(2, 25000, 8);  // 12 kHz PWM, 8-bit resolution
  ledcAttachPin(CPU_PWM, 2);
  ledcWrite(2, 0);

  ledcSetup(3, 25000, 8);  // 12 kHz PWM, 8-bit resolution
  ledcAttachPin(GPU_PWM, 3);
  ledcWrite(3, 0);

  ledcSetup(4, 25000, 8);  // 12 kHz PWM, 8-bit resolution
  ledcAttachPin(SSD_PWM, 4);
  ledcWrite(4, 0);

  attachInterrupt(CPU_TACHO, countCpuPulse, FALLING);
  attachInterrupt(GPU_TACHO, countGpuPulse, FALLING);
  attachInterrupt(SSD_TACHO, countSsdPulse, FALLING);

  cpuRelay.begin();
  gpuRelay.begin();
  ssdRelay.begin();
}

void loopFans() {
  Temperature newValue = Temperature(cpu.temperature, gpu.temperature, ssd.temperature);
  updateThermoBuffer(newValue);
  Serial.println();
  Serial.print("CPU_RPM:");
  Serial.println(calculateCpuRPM(cpuPulses, millis()));
  Serial.print("GPU_RPM:");
  Serial.println(calculateGpuRPM(gpuPulses, millis()));
  Serial.print("SSD_RPM:");
  Serial.println(calculateSsdRPM(ssdPulses, millis()));

  if (cpuCompValue > cpuMin) {
    cpuRelay.turnOn();
    if (!cpuFlag) {
      cpuFlag = true;
      ledcWrite(2, pwm_max);
    } else {
      ledcWrite(2, calcPwmDuty(min(cpuCompValue, cpuMax), cpuMin, cpuMax, 10, pwm_max));
    }
    // Serial.print("cpu_pwm:");
    // Serial.println(OCR3A * 100 / pwm_max);
  } else {
    cpuFlag = false;
    cpuRelay.turnOff();
  }
  if (gpuCompValue > gpuMin) {
    gpuRelay.turnOn();
    if (!gpuFlag) {
      gpuFlag = true;
      ledcWrite(3, pwm_max);
    } else {
      ledcWrite(3, calcPwmDuty(min(gpuCompValue, gpuMax), gpuMin, gpuMax, 10, pwm_max));
    }
    // Serial.print("gpu_pwm:");
    // Serial.println(OCR4A * 100 / pwm_max);
  } else {
    gpuFlag = false;
    gpuRelay.turnOff();
  }
  if (ssdCompValue > ssdMin) {
    ssdRelay.turnOn();
    if (!ssdFlag) {
      ssdFlag = true;
      ledcWrite(4, pwm_max);
    } else {
      ledcWrite(4, calcPwmDuty(min(ssdCompValue, ssdMax), ssdMin, ssdMax, 30, pwm_max));
    }
    // Serial.print("ssd_pwm:");
    // Serial.println(OCR1A * 100 / pwm_max);
  } else {
    ssdFlag = false;
    ssdRelay.turnOff();
  }
}
