#include <Wire.h>
#include <MPU6050_light.h>
#include <Adafruit_BME280.h>
#include <BasicLinearAlgebra.h>

using namespace BLA;
#define SEA_LEVEL_PRESSURE 997.96

float t_step = 0.05; // timestep in seconds

BLA::Matrix<3,1> x = {0, 0, 0}; // State vector (altitude, velocity, acceleration)
BLA::Matrix<2,1> m; // Measured sensor values (altitude, acceleration)

BLA::Matrix<3,3> A = { 1, t_step, 0.5*sq(t_step),
                      0, 1, t_step,
                      0, 0, 1 }; // maps previous to next state 

BLA::Matrix<2,3> H = { 1, 0, 0,   // maps x to m
                       0, 0, 1 };
BLA::Matrix<2,2> R = { 0.03, 0,
                       0, 0.00105 }; // Noise covariance from the sensors (diagonal with variances)
BLA::Matrix<3,3> Q = { 0.005, 0, 0,
                      0, 0., 0,
                      0, 0, 0.005 };; // Process noise covariance matrix
BLA::Matrix<3,3> I = { 1, 0, 0,
                      0, 1, 0,
                      0, 0, 1 };
BLA::Matrix<3,2> K;

BLA::Matrix<3,3> P = { 1, 0, 0,
                      0, 1, 0,
                      0, 0, 1 };

unsigned long dt;
unsigned long timer = 0;
float ax, alt, alt_ref;

Adafruit_BME280 bme;
MPU6050 mpu(Wire);

void setup() {
  Serial.begin(9600);
  bme.begin(0x76);
  Wire.begin();
  mpu.begin();
  delay(1000);
  mpu.calcOffsets(true);
  alt_ref = bme.readAltitude(SEA_LEVEL_PRESSURE);
  delay(500);

}
void loop() {
  mpu.update();
  dt = millis() - timer;
  if (dt > t_step*1000){
    //Read altitude and acceleration from sensors: 
    ax = mpu.getAccX() * 9.806; // convert g in m/s^2
    alt = bme.readAltitude(SEA_LEVEL_PRESSURE) - alt_ref;

    m(0) = alt;
    m(1) = ax;

    for (int i = 0; i < 20; i++){
    K = P*~H*Inverse(H*P*~H + R);
    P = (I - K*H)*P;
    P = A*P*~A + Q;
    } // K and P are determined after 10 iterations.
    
    x = A*x;
    x = x + K*(m - H*x); //ora x contiene i valori di accelerazione e altitudine ottimali. 

    // Serial.print("Alt : ");
    Serial.print(1);
    Serial.print('\t');
    Serial.print(-1);
    Serial.print('\t');
    Serial.print(alt);
    Serial.print('\t');
    // Serial.print("\tAlt_Kalman: ");
    Serial.println(x(0));

    timer = millis();

    delay(100);
  }

}