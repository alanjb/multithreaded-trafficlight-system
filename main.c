#include<sys/utsname.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<fcntl.h>
#include<pthread.h>

//GLOBAL CONSTANTS
#define SYS_GPIO_PATH "/sys/class/gpio/gpio"
#define VALUE_PROPERTY "/value"
#define LIGHT_ON "1"
#define LIGHT_OFF "0"
#define PIN_STATE_SIZE 2
#define PINNUM_SIZE 2
#define GPIOVALUEPATH_SIZE 28
#define VALUE_ENDPOINT_SIZE 7
#define TWO_MINUTES 120
#define TEN_SECONDS 10
#define TEN_NUM 10
#define FIVE_SECONDS 5
#define ZERO_NUM 0
#define ONE_NUM 1
#define TWO_NUM 2
#define FIVE_NUM 5
#define MILLION_NUM 1000000
#define FIVE_SECS 5000000
#define NEGATIVE_ONE_NUM -1

//PIN NUMS FOR TRAFFIC LIGHT 1
#define TRAFFICLIGHT1_REDLIGHT "69"
#define TRAFFICLIGHT1_YELLOWLIGHT "45"
#define TRAFFICLIGHT1_GREENLIGHT "66"

//PIN NUMS FOR TRAFFIC LIGHT 2
#define TRAFFICLIGHT2_REDLIGHT "44"
#define TRAFFICLIGHT2_YELLOWLIGHT "68"
#define TRAFFICLIGHT2_GREENLIGHT "67"

//PIN NUM FOR TRAFFIC LIGHT 1 WAIT SENSOR
#define WAIT_SENSOR1_PIN "47"

//PIN NUM FOR TRAFFIC LIGHT 2 WAIT SENSOR
#define WAIT_SENSOR2_PIN "26"

//TYPEDEFS
typedef char PinStateType[PIN_STATE_SIZE];
typedef char *PinNumType[PINNUM_SIZE];
typedef char GpioValuePathType[GPIOVALUEPATH_SIZE];
typedef char ValueEndType[VALUE_ENDPOINT_SIZE];

//GLOBAL VARIABLES
int8_t globalTimer = TEN_NUM;
int8_t trafficLightTurn1 = ZERO_NUM;
int8_t trafficLightTurn2 = ZERO_NUM;

//0 = reset value, 1 = trafficLight1's turn, 2 = trafficLight2's turn
int8_t whoseNextTurn = ZERO_NUM;

//MUTEXES
pthread_mutex_t globalTimer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trafficLight1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trafficLight2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trafficLightTurn1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trafficLightTurn2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t whoseNextTurn_mutex = PTHREAD_MUTEX_INITIALIZER;

//CONDITIONS
pthread_cond_t globalTimer_cond = PTHREAD_COND_INITIALIZER;

//Wait Sensor Struct
struct WaitSensor {
	PinNumType pinNum;
};

//Initial State of Wait Sensor 1
struct WaitSensor waitSensor1 = {
	.pinNum = WAIT_SENSOR1_PIN,
};

//Initial State of Wait Sensor 2
struct WaitSensor waitSensor2 = {
	.pinNum = WAIT_SENSOR2_PIN,
};

//Traffic Light Struct
struct TrafficLight {
	PinStateType redLightState;
	PinNumType redLightPinNum;

	PinStateType yellowLightState;
	PinNumType yellowLightPinNum;

	PinStateType greenLightState;
	PinNumType greenLightPinNum;
};

//FUNCTION DEFINITIONS
void handleConfiguringGpioPin(PinStateType pinState, PinNumType pinNum);
void printSystemData();
void printData();
void *startTrafficLight1SensorThread();
void *startTrafficLight2SensorThread();
void updateTrafficLight1(int8_t waitSensorDetected);
void *handleTrafficLight1();
void updateTrafficLight2(int8_t waitSensorDetected);
void *handleTrafficLight2();
void *updateGlobalTimer();
void resetGlobalTimer(int time);
void updateLightState(struct TrafficLight trafficLight, char *state);
char *buildGpioValuePath(char gpioValuePath[GPIOVALUEPATH_SIZE], PinNumType pinNum);

//Initial State of TrafficLight1
struct TrafficLight trafficLight1 = {
	.redLightState = LIGHT_OFF,
	.redLightPinNum = TRAFFICLIGHT1_REDLIGHT,

	.yellowLightState = LIGHT_OFF,
	.yellowLightPinNum = TRAFFICLIGHT1_YELLOWLIGHT,

	.greenLightState = LIGHT_OFF,
	.greenLightPinNum = TRAFFICLIGHT1_GREENLIGHT,
};

//Initial State of TrafficLight2
struct TrafficLight trafficLight2 = {
	.redLightState = LIGHT_OFF,
	.redLightPinNum = TRAFFICLIGHT2_REDLIGHT,

	.yellowLightState = LIGHT_OFF,
	.yellowLightPinNum = TRAFFICLIGHT2_YELLOWLIGHT,

	.greenLightState = LIGHT_OFF,
	.greenLightPinNum = TRAFFICLIGHT2_GREENLIGHT,
};

//PROGRAM STARTS HERE
int main(){
	printSystemData();

	pthread_t trafficLight1_thread, trafficLight2_thread, globalTimer_thread, trafficLight1_sensor_thread, trafficLight2_sensor_thread;

	printf("Starting threads...\n\n\n");

	//THREADS
	pthread_create(&trafficLight1_thread, NULL, &handleTrafficLight1, NULL);
	pthread_create(&trafficLight2_thread, NULL, &handleTrafficLight2, NULL);
	pthread_create(&globalTimer_thread, NULL, &updateGlobalTimer, NULL);
	pthread_create(&trafficLight1_sensor_thread, NULL, &startTrafficLight1SensorThread, NULL);
	pthread_create(&trafficLight2_sensor_thread, NULL, &startTrafficLight2SensorThread, NULL);

	//JOINS
	pthread_join(trafficLight1_thread, NULL);
	pthread_join(trafficLight2_thread, NULL);
	pthread_join(globalTimer_thread, NULL);
	pthread_join(trafficLight1_sensor_thread, NULL);
	pthread_join(trafficLight2_sensor_thread, NULL);

	return ZERO_NUM;
}

//print system name, node name, and processor at the state of the program
void printSystemData(){
    struct utsname uts;

	if(uname(&uts) < ZERO_NUM){
		printf("Error retrieving system information...");
	}

	else {
		printf("System name: %s\n", uts.sysname);
		printf("Node name: %s\n", uts.nodename);
        printf("Processor Machine: %s\n\n\n", uts.machine);
	}
}

//print data for traffic light 1
void printTrafficLight1Data(){
	printf("\n\n\ntrafficLight1 red: %s\n", trafficLight1.redLightState);
	printf("trafficLight1 yellow: %s\n", trafficLight1.yellowLightState);
	printf("trafficLight1 green: %s\n\n\n", trafficLight1.greenLightState);
}

//print data for traffic light 2
void printTrafficLight2Data(){
	printf("\ntrafficLight2 red: %s\n", trafficLight2.redLightState);
	printf("trafficLight2 yellow: %s\n", trafficLight2.yellowLightState);
	printf("trafficLight2 green: %s\n\n\n", trafficLight2.greenLightState);
}

void handleConfiguringGpioPin(PinStateType pinState, PinNumType pinNum){
	int8_t fd = ZERO_NUM;
	char gpioValuePath[GPIOVALUEPATH_SIZE];

	//create the gpio value path for this pin
	buildGpioValuePath(gpioValuePath, pinNum);

	//open and write pin
	fd = open(gpioValuePath, O_WRONLY);
	write(fd, pinState, ONE_NUM);
	close(fd);
}

char *buildGpioValuePath(char gpioValuePath[GPIOVALUEPATH_SIZE], PinNumType pinNum){
	ValueEndType value_part2;

	//concat the gpio value path + pin num + value end point to create the path to the value of the gpio pin to be configured
	strcpy(gpioValuePath, SYS_GPIO_PATH);
	strcpy(value_part2, VALUE_PROPERTY);
	strcat(gpioValuePath, *pinNum);
	strcat(gpioValuePath, value_part2);
	
	gpioValuePath[GPIOVALUEPATH_SIZE] = '\0';

	return gpioValuePath;
}

void *handleTrafficLight1(){
	printf("\n\n\nStarting Traffic Light 1...\n\n\n");

	resetGlobalTimer(TEN_SECONDS);

	for(;;){
		pthread_mutex_lock(&globalTimer_mutex);
		while(globalTimer != ZERO_NUM){
			//await for timer to complete
			pthread_cond_wait(&globalTimer_cond, &globalTimer_mutex);
		}
		pthread_mutex_unlock(&globalTimer_mutex);
		updateTrafficLight1(TWO_NUM);
		usleep(MILLION_NUM);
    }
}

void *handleTrafficLight2(){
	printf("\n\n\nStarting Traffic Light 2...\n\n\n");

	resetGlobalTimer(TEN_SECONDS);

	for(;;){
		pthread_mutex_lock(&globalTimer_mutex);
		while(globalTimer != ZERO_NUM){
			//await for timer to complete
			pthread_cond_wait(&globalTimer_cond, &globalTimer_mutex);
		}
		pthread_mutex_unlock(&globalTimer_mutex);
		updateTrafficLight2(TWO_NUM);
		usleep(MILLION_NUM);
    }
}

void updateTrafficLight1(int8_t waitSensorDetected){ 
	int8_t trafficLight1_redLight_isOn = strcmp(trafficLight1.redLightState, LIGHT_ON);
	int8_t trafficLight1_redLight_isOff = strcmp(trafficLight1.redLightState, LIGHT_OFF);

	int8_t trafficLight1_yellowLight_isOn = strcmp(trafficLight1.yellowLightState, LIGHT_ON);
	int8_t trafficLight1_yellowLight_isOff = strcmp(trafficLight1.yellowLightState, LIGHT_OFF);

	int8_t trafficLight1_greenLight_isOn = strcmp(trafficLight1.greenLightState, LIGHT_ON);
	int8_t trafficLight1_greenLight_isOff = strcmp(trafficLight1.greenLightState, LIGHT_OFF);

	//run this statement if other traffic light sensor activated. This will transition this light to red
	if(waitSensorDetected == ZERO_NUM){
		pthread_mutex_lock(&trafficLightTurn1_mutex);
		trafficLightTurn1 = ZERO_NUM;
		pthread_mutex_unlock(&trafficLightTurn1_mutex); 

		pthread_mutex_lock(&whoseNextTurn_mutex);
		whoseNextTurn = TWO_NUM;
		pthread_mutex_unlock(&whoseNextTurn_mutex);
	}

	//first entrance, turn to red
	if(trafficLight1_redLight_isOff == ZERO_NUM && trafficLight1_greenLight_isOff == ZERO_NUM){
		pthread_mutex_lock(&trafficLight1_mutex);

		strcpy(trafficLight1.redLightState, LIGHT_ON);
		handleConfiguringGpioPin(trafficLight1.redLightState, trafficLight1.redLightPinNum);

		printTrafficLight1Data();

		pthread_mutex_unlock(&trafficLight1_mutex);

		pthread_mutex_lock(&trafficLightTurn1_mutex);
		trafficLightTurn1 = ONE_NUM;
		pthread_mutex_unlock(&trafficLightTurn1_mutex);

		pthread_mutex_lock(&whoseNextTurn_mutex);
		whoseNextTurn = ONE_NUM;
		pthread_mutex_unlock(&whoseNextTurn_mutex);

		resetGlobalTimer(TEN_SECONDS);
	}
	else if((trafficLightTurn1 == ONE_NUM && whoseNextTurn == ONE_NUM) || waitSensorDetected == ONE_NUM){
		pthread_mutex_lock(&trafficLight1_mutex);

		strcpy(trafficLight1.redLightState, LIGHT_OFF);
		handleConfiguringGpioPin(trafficLight1.redLightState, trafficLight1.redLightPinNum);

		strcpy(trafficLight1.greenLightState, LIGHT_ON);
		handleConfiguringGpioPin(trafficLight1.greenLightState, trafficLight1.greenLightPinNum);

		printTrafficLight1Data();

		pthread_mutex_unlock(&trafficLight1_mutex);

		pthread_mutex_lock(&trafficLightTurn1_mutex);
		trafficLightTurn1 = ZERO_NUM;
		pthread_mutex_unlock(&trafficLightTurn1_mutex);

		pthread_mutex_lock(&whoseNextTurn_mutex);
		whoseNextTurn = ZERO_NUM;
		pthread_mutex_unlock(&whoseNextTurn_mutex);

		resetGlobalTimer(TWO_MINUTES);
	}
	else if((trafficLightTurn1 == ZERO_NUM && whoseNextTurn == ZERO_NUM) || waitSensorDetected == ZERO_NUM) {
		pthread_mutex_lock(&trafficLight1_mutex);

		strcpy(trafficLight1.greenLightState, LIGHT_OFF);
		handleConfiguringGpioPin(trafficLight1.greenLightState, trafficLight1.greenLightPinNum);

		strcpy(trafficLight1.yellowLightState, LIGHT_ON);
		handleConfiguringGpioPin(trafficLight1.yellowLightState, trafficLight1.yellowLightPinNum);

		pthread_mutex_lock(&trafficLightTurn1_mutex);
		trafficLightTurn1 = ONE_NUM;
		pthread_mutex_unlock(&trafficLightTurn1_mutex);

		printTrafficLight1Data();

		resetGlobalTimer(TEN_SECONDS);

		usleep(FIVE_SECS);

		strcpy(trafficLight1.yellowLightState, LIGHT_OFF);
		handleConfiguringGpioPin(trafficLight1.yellowLightState, trafficLight1.yellowLightPinNum);

		strcpy(trafficLight1.redLightState, LIGHT_ON);
		handleConfiguringGpioPin(trafficLight1.redLightState, trafficLight1.redLightPinNum);

		printTrafficLight1Data();

		pthread_mutex_unlock(&trafficLight1_mutex);

		pthread_mutex_lock(&whoseNextTurn_mutex);
		whoseNextTurn = TWO_NUM;
		pthread_mutex_unlock(&whoseNextTurn_mutex);

		resetGlobalTimer(TEN_SECONDS);
	}
}

void updateTrafficLight2(int8_t waitSensorDetected){
	int8_t trafficLight2_redLight_isOn = strcmp(trafficLight2.redLightState, LIGHT_ON);
	int8_t trafficLight2_redLight_isOff = strcmp(trafficLight2.redLightState, LIGHT_OFF);

	int8_t trafficLight2_yellowLight_isOn = strcmp(trafficLight2.yellowLightState, LIGHT_ON);
	int8_t trafficLight2_yellowLight_isOff = strcmp(trafficLight2.yellowLightState, LIGHT_OFF);

	int8_t trafficLight2_greenLight_isOn = strcmp(trafficLight2.greenLightState, LIGHT_ON);
	int8_t trafficLight2_greenLight_isOff = strcmp(trafficLight2.greenLightState, LIGHT_OFF);

	if(waitSensorDetected == ZERO_NUM){

		//reset this traffic lights turn to 0 since we are going through their turn now.
		pthread_mutex_lock(&trafficLightTurn2_mutex);
		trafficLightTurn2 = ZERO_NUM;
		pthread_mutex_unlock(&trafficLightTurn2_mutex);

		pthread_mutex_lock(&whoseNextTurn_mutex);
		whoseNextTurn = ONE_NUM;
		pthread_mutex_unlock(&whoseNextTurn_mutex);
	}

	if(trafficLight2_redLight_isOff == ZERO_NUM && trafficLight2_greenLight_isOff == ZERO_NUM){
		pthread_mutex_lock(&trafficLight2_mutex);

		strcpy(trafficLight2.redLightState, LIGHT_ON);
		handleConfiguringGpioPin(trafficLight2.redLightState, trafficLight2.redLightPinNum);

		printTrafficLight2Data();

		pthread_mutex_unlock(&trafficLight2_mutex);

		pthread_mutex_lock(&trafficLightTurn2_mutex);
		trafficLightTurn2 = ONE_NUM;
		pthread_mutex_unlock(&trafficLightTurn2_mutex);
	}
	else if((trafficLightTurn2 == ONE_NUM && whoseNextTurn == TWO_NUM) || waitSensorDetected == ONE_NUM){
		pthread_mutex_lock(&trafficLight2_mutex);
	
		strcpy(trafficLight2.redLightState, LIGHT_OFF);
		handleConfiguringGpioPin(trafficLight2.redLightState, trafficLight2.redLightPinNum);

		strcpy(trafficLight2.greenLightState, LIGHT_ON);
		handleConfiguringGpioPin(trafficLight2.greenLightState, trafficLight2.greenLightPinNum);

		printTrafficLight2Data();

		pthread_mutex_unlock(&trafficLight2_mutex);

		pthread_mutex_lock(&trafficLightTurn2_mutex);
		trafficLightTurn2 = ZERO_NUM;
		pthread_mutex_unlock(&trafficLightTurn2_mutex);

		resetGlobalTimer(TWO_MINUTES);
	}
	else if((waitSensorDetected == ZERO_NUM || trafficLightTurn2 == ZERO_NUM  && whoseNextTurn == TWO_NUM)) {
		pthread_mutex_lock(&trafficLight2_mutex);

		strcpy(trafficLight2.greenLightState, LIGHT_OFF);
		handleConfiguringGpioPin(trafficLight2.greenLightState, trafficLight2.greenLightPinNum);

		strcpy(trafficLight2.yellowLightState, LIGHT_ON);
		handleConfiguringGpioPin(trafficLight2.yellowLightState, trafficLight2.yellowLightPinNum);

		pthread_mutex_lock(&trafficLightTurn2_mutex);
		trafficLightTurn2 = ONE_NUM;
		pthread_mutex_unlock(&trafficLightTurn2_mutex);

		printTrafficLight2Data();

		resetGlobalTimer(TEN_SECONDS);

		//wait 5secs for yellow light
		usleep(FIVE_SECS);

		strcpy(trafficLight2.yellowLightState, LIGHT_OFF);
		handleConfiguringGpioPin(trafficLight2.yellowLightState, trafficLight2.yellowLightPinNum);

		strcpy(trafficLight2.redLightState, LIGHT_ON);
		handleConfiguringGpioPin(trafficLight2.redLightState, trafficLight2.redLightPinNum);

		printTrafficLight2Data();

		pthread_mutex_unlock(&trafficLight2_mutex);

		pthread_mutex_lock(&whoseNextTurn_mutex);
		whoseNextTurn = ONE_NUM;
		pthread_mutex_unlock(&whoseNextTurn_mutex);

		resetGlobalTimer(TEN_SECONDS);
	}
}

void resetGlobalTimer(int time){
	pthread_mutex_lock(&globalTimer_mutex);
	//set the timer based on the passed in time, depending on which light is turned on
	globalTimer = time;
	pthread_mutex_unlock(&globalTimer_mutex);
}

void *updateGlobalTimer(){
	printf("\n\n\nStarting Global Timer...\n\n\n");

	while(ONE_NUM){
		pthread_mutex_lock(&globalTimer_mutex);	
		if(globalTimer != ZERO_NUM){
			//count down timer
			globalTimer--;
			printf("\n\n\nGlobal Timer Value...%d\n\n\n", globalTimer);
		}
		else {
			globalTimer = TEN_SECONDS;
		}
		//broadcast to all signals awaiting if the timer is complete
		pthread_cond_broadcast(&globalTimer_cond);
		pthread_mutex_unlock(&globalTimer_mutex);
		usleep(MILLION_NUM);
	}
}

void *startTrafficLight1SensorThread(){
	printf("\n\n\nStarting Traffic Light 1 Wait Sensor Thread\n\n\n");

	resetGlobalTimer(TEN_SECONDS);

	int8_t fd=ZERO_NUM;
	int8_t count=ZERO_NUM;
	char c[TEN_NUM];
	char gpioValuePath[GPIOVALUEPATH_SIZE];

	buildGpioValuePath(gpioValuePath, waitSensor1.pinNum);

	while(ONE_NUM){
		if ((fd = open(gpioValuePath, O_RDONLY)) == NEGATIVE_ONE_NUM){
			printf("File opening failed...\n");
			exit(0);
		}
		else {
			if (read(fd, c, sizeof(c)) == NEGATIVE_ONE_NUM){
   				printf("Error while reading file...\n");
   				exit(ZERO_NUM);
  			}	
			else {
   				printf("\n\n\nWait Sensor 1 State: %s\n\n\n", c);

				close(fd);

				//check if the sensor is on or off
				int8_t traffic_light1_wait_sensor_isOn = strcmp(c, "1\n");
				int8_t traffic_light1_wait_sensor_isOff = strcmp(c, "0\n");
 
				pthread_mutex_lock(&trafficLightTurn1_mutex);	
				if(trafficLightTurn1 == ONE_NUM && traffic_light1_wait_sensor_isOn == ZERO_NUM){
					pthread_mutex_unlock(&trafficLightTurn1_mutex);	
					printf("\n\n\nWait Sensor 1 Pressed...\n\n\n");
					count++;
				}
				pthread_mutex_unlock(&trafficLightTurn1_mutex);	

				//reset the count if pressed and released under 5 seconds
				if(count >= ONE_NUM && count < FIVE_NUM && traffic_light1_wait_sensor_isOff == ZERO_NUM){
					printf("\n\n\nWait Sensor 1 Resetting...\n\n\n");
					count = ZERO_NUM;
				}

				//if held for 5 seconds, activate wait sensor and handle transitioning between lights
				pthread_mutex_lock(&globalTimer_mutex);	
				if(count == FIVE_SECONDS && globalTimer > 5){
					pthread_mutex_unlock(&globalTimer_mutex);	
					printf("\n\n\nWait Sensor 1 Activated...\n\n\n");
					resetGlobalTimer(TEN_SECONDS);	
					
					updateTrafficLight2(ZERO_NUM);
					updateTrafficLight1(ONE_NUM);

					resetGlobalTimer(TWO_MINUTES);	

					count = ZERO_NUM;
				}
				pthread_mutex_unlock(&globalTimer_mutex);	
 	 		}
		}
		usleep(MILLION_NUM);
	}
}

void *startTrafficLight2SensorThread(){
	printf("\n\n\nStarting Traffic Light 2 Wait Sensor Thread\n\n\n");

	resetGlobalTimer(TEN_SECONDS);

	int8_t fd=ZERO_NUM;
	int8_t count=ZERO_NUM;
	char c[TEN_NUM];
	char gpioValuePath[GPIOVALUEPATH_SIZE];

	buildGpioValuePath(gpioValuePath, waitSensor2.pinNum);

	while(ONE_NUM){
		if ((fd = open(gpioValuePath, O_RDONLY)) == NEGATIVE_ONE_NUM){
			printf("File opening failed...\n");
			exit(0);
		}
		else {
			if (read(fd, c, sizeof(c)) == NEGATIVE_ONE_NUM){
   				printf("Error while reading file...\n");
   				exit(ZERO_NUM);
  			}	
			else {
   				printf("\n\n\nWait Sensor 2 State: %s\n\n\n", c);

				close(fd);

				//check if the sensor is on or off
				int8_t traffic_light2_wait_sensor_isOn = strcmp(c, "1\n");
				int8_t traffic_light2_wait_sensor_isOff = strcmp(c, "0\n");

				pthread_mutex_lock(&trafficLightTurn2_mutex);	
				if(trafficLightTurn2 == ONE_NUM && traffic_light2_wait_sensor_isOn == ZERO_NUM){
					pthread_mutex_unlock(&trafficLightTurn2_mutex);	
					printf("Wait Sensor 2 Pressed...");
					count++;
				}
				pthread_mutex_unlock(&trafficLightTurn2_mutex);	

				//reset the count if pressed and released under 5 seconds
				if(count >= ONE_NUM && count < FIVE_NUM && traffic_light2_wait_sensor_isOff == ZERO_NUM){
					printf("\n\n\nWait Sensor 2 Resetting...\n\n\n");
					count = ZERO_NUM;
				}

				pthread_mutex_lock(&globalTimer_mutex);
				if(count == FIVE_SECONDS && globalTimer > FIVE_SECONDS){
					pthread_mutex_unlock(&globalTimer_mutex);
					printf("Wait Sensor 2 Activated...");
					resetGlobalTimer(TEN_SECONDS);	
					
					updateTrafficLight1(ZERO_NUM);
					updateTrafficLight2(ONE_NUM);

					resetGlobalTimer(TWO_MINUTES);	

					count = ZERO_NUM;
				}
				pthread_mutex_unlock(&globalTimer_mutex);
 	 		}
		}
		usleep(MILLION_NUM);
	}
}