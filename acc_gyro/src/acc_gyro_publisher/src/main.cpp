#include "ros/ros.h"
#include "std_msgs/String.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 mpu;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

void setup() {
	// initialize device
	printf("Initializing I2C devices...\n");
	mpu.initialize();

	// verify connection
	printf("Testing device connections...\n");
	printf(mpu.testConnection() ? "MPU6050 connection successful\n" : "MPU6050 connection failed\n");

	// load and configure the DMP
	printf("Initializing DMP...\n");
	devStatus = mpu.dmpInitialize();

	// make sure it worked (returns 0 if so)
	if (devStatus == 0) {
		// turn on the DMP, now that it's ready
		printf("Enabling DMP...\n");
		mpu.setDMPEnabled(true);

		// enable Arduino interrupt detection
		//Serial.println(F("Enabling interrupt detection (Arduino external interrupt 0)..."));
		//attachInterrupt(0, dmpDataReady, RISING);
		mpuIntStatus = mpu.getIntStatus();

		// set our DMP Ready flag so the main loop() function knows it's okay to use it
		printf("DMP ready!\n");
		dmpReady = true;

		// get expected DMP packet size for later comparison
		packetSize = mpu.dmpGetFIFOPacketSize();
	} else {
		// ERROR!
		// 1 = initial memory load failed
		// 2 = DMP configuration updates failed
		// (if it's going to break, usually the code will be 1)
		printf("DMP Initialization failed (code %d)\n", devStatus);
	}
}

void loop() {
	// if programming failed, don't try to do anything
	if (!dmpReady) return;
	// get current FIFO count
	fifoCount = mpu.getFIFOCount();

	if (fifoCount == 1024) {
		// reset so we can continue cleanly
		mpu.resetFIFO();
		printf("FIFO overflow!\n");

		// otherwise, check for DMP data ready interrupt (this should happen frequently)
	} else if (fifoCount >= 42) {
		// read a packet from FIFO
		mpu.getFIFOBytes(fifoBuffer, packetSize);

		// display initial world-frame acceleration, adjusted to remove gravity
		// and rotated based on known orientation from quaternion
		mpu.dmpGetQuaternion(&q, fifoBuffer);
		mpu.dmpGetAccel(&aa, fifoBuffer);
		mpu.dmpGetGravity(&gravity, &q);
		mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
		printf("aworld %6d %6d %6d    ", aaWorld.x, aaWorld.y, aaWorld.z);
	}
}

int main(int argc, char **argv)
{
	setup();
	usleep(100000);

	ros::init(argc, argv, "acc_gyro_publisher");

	ros::NodeHandle n;

	ros::Publisher acc_gyro_pub = n.advertise<std_msgs::String>("acc_gyro_publisher", 1000);

	int count = 0;
	while (ros::ok())
	{

		loop();

		std_msgs::String msg;

		std::stringstream ss;
		ss << "hello world " << count;
		msg.data = ss.str();

		ROS_INFO("%s", msg.data.c_str());

		acc_gyro_pub.publish(msg);

		ros::spinOnce();

		++count;
	}


	return 0;
}
