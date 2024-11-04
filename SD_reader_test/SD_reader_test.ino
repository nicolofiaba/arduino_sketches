#include <SPI.h>
#include <SD.h>

File myFile;

void setup() {
  Serial.begin(9600);
  if (!SD.begin(10)) {
    Serial.println("SD initialization failed!");
    while(1);
  }
  Serial.println("SD initialization done!");

  if (SD.exists("example.txt")) {
    Serial.println("example.txt already exists.");
    while(1);
  } else {
    Serial.println("example.txt doesn't exist.");
    delay(1000);
  }

  Serial.println("Creating example.txt  file..");
  myFile = SD.open("example.txt", FILE_WRITE);
  myFile.write("Hello!");
  Serial.println("Done!");
  myFile.close();
}

void loop() {
  // put your main code here, to run repeatedly:

}
