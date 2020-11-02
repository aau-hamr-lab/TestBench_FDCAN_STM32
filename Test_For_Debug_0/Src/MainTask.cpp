/* Copyright (C) 2020-2020 Juan de Dios Flores Mendez. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the MIT License
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the MIT License for further details.
 *
 * Contact information
 * ------------------------------------------
 * Juan de Dios Flores Mendez
 * Web      :  https://github.com/jdios89
 * e-mail   :  juan.dios.flores@gmail.com
 * ------------------------------------------
 */

#include "MainTask.h"
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "main.h"
//#include "MemoryManagement.h"
#include "FDCAN.h" // Periphirial library
#include "Timer.h"
//#include "PD4Cxx08.h"
//#include "CAN_CMD.h"
#include "NANOTEC_Bus.h" // Communication library
#include "NANOTEC.h" // Motor specific library

int32_t encoder1 = 0;
int32_t encoder2 = 0;
int32_t encoder3 = 0;

void testTask() {
	float desd = 0.0f;
//	FDCAN * fdcantest22 = new FDCAN(); // port object
}
void MainTask(void * pvParameters) {
	/* Use this task to:
	 * - Create objects for each module
	 *     (OBS! It is very important that objects created with "new"
	 *      only happens within a thread due to the usage of the FreeRTOS managed heap)
	 * - Link any modules together if necessary
	 * - Create message exchange queues and/or semaphore
	 * - (Create) and start threads related to modules
	 *
	 * Basically anything related to starting the system should happen in this thread and NOT in the main() function !!!
	 */

	float d = 2;
//	FDCAN fdcantest2; // port object
	FDCAN * fdcantest = new FDCAN(); // port object

	NANOTEC * motor1 = new NANOTEC(fdcantest, (uint8_t) 0x1, 2.0f, 3.54f / 4.2f,
			1.0f, 10000.0, 30.0);
	NANOTEC * motor2 = new NANOTEC(fdcantest, (uint8_t) 0x2, 2.0f, 3.54f / 4.2f,
			1.0f, 10000.0, 30.0);
	NANOTEC * motor3 = new NANOTEC(fdcantest, (uint8_t) 0x3, 2.0f, 3.54f / 4.2f,
			1.0f, 10000.0, 30.0);
	fdcantest->WriteDummyData((uint8_t) 2);
	fdcantest->WriteMessage((uint32_t) 0x21, (uint8_t) 0x4, (uint8_t) 0x3,
			(uint8_t) 0x2, (uint8_t) 0x3, (uint8_t) 0x2, (uint8_t) 0x0, (uint8_t) 0x0,
			(uint8_t) 0x0, (uint8_t) 0x0);
	fdcantest->WriteMessage((uint32_t) 0x0, (uint8_t) 0x2, (uint8_t) 0x81,
			(uint8_t) 0x1, (uint8_t) 0x0, (uint8_t) 0x0, (uint8_t) 0x0, (uint8_t) 0x0,
			(uint8_t) 0x0, (uint8_t) 0x0);
	fdcantest->Read();
	/* For now configure the motors after creation */
	motor1->Configure();
	motor2->Configure();
	motor3->Configure();

	/* Initialize microseconds timer */
	Timer * microsTimer = new Timer(Timer::TIMER6, 1000000); // create a 1 MHz counting timer used for micros() timing

	/* Control variables */
	TickType_t xLastWakeTime;
	uint32_t prevTimerValue; // used for measuring dt
	float timestamp, dt_compute, dt_compute2;
	float SampleRate = 120; // can improve

	/* Controller loop time / sample rate */
	TickType_t loopWaitTicks = configTICK_RATE_HZ / SampleRate;
	/* Main control loop */
	xLastWakeTime = xTaskGetTickCount();
	prevTimerValue = microsTimer->Get();
	float pos[3] = { 0.0f, 0.0f, 0.0f };
	float pos_d[3] = { 0.0f, 0.0f, 0.0f };
	float pos_l[3] = { 0.0f, 0.0f, 0.0f };
	float error_pos[3] = { 0.0f, 0.0f, 0.0f };
	float error_pos_l[3] = { 0.0f, 0.0f, 0.0f };
	float output[3] = { 0.0f, 0.0f, 0.0f };
	float kp_c[3] = { 0.0f, 0.0f, 0.0f };
	float kd_c[3] = { 0.0f, 0.0f, 0.0f };
	float ki_c[3] = { 0.0f, 0.0f, 0.0f };
	float kp = 0.08;
	float kd = 0.0; // 0.2;
	float ki = 0.0;
//	float dt = 0.004;

	/*
	 while (1) {
	 HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
	 osDelay(348);
	 }
	 */
	// Compute control
	while (1) {
		/* Wait until time has been reached to make control loop periodic */
		vTaskDelayUntil(&xLastWakeTime, loopWaitTicks);
		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);

///		osDelay(1);
///		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
//		osDelay(3);
		// Reference generator
		float amplitude[3] = { 3.5, 1.0, 1.6 };
		float freq[3] = { 0.23, 0.5, 1.0 };
		for (int i = 0; i < 3; i++) {
			float PI = 3.14159265;
			float time_ms_ = (float) HAL_GetTick(); //uint32_t
			pos_d[i] = amplitude[i] * sinf(2.0f * PI * freq[i] * time_ms_ / 1000.0f);
		}
		for (int i = 0; i < 3; i++){
			pos_d[i] = 0.0;
		}
		// Control computation
		for (int i = 0; i < 3; i++) {
			pos_l[i] = pos[i];
			switch (i) {
			case 0:
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
				pos[i] = motor1->GetAngle();
			//	HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
				break;
			case 1:
			//	HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
				pos[i] = motor2->GetAngle();
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
				break;
			case 2:
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
				pos[i] = motor3->GetAngle();
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
				break;
			}

//			uint32_t timerPrev = HAL_tic();
			volatile float posv[3] = {0.0, 0.0, 0.0};
			for (int i = 0; i < 3; i++) posv[i] = pos[i];
			float dt = microsTimer->GetDeltaTime(prevTimerValue);
			prevTimerValue = microsTimer->Get();
			timestamp = microsTimer->GetTime();
//			pos[i] = NANOTECS[i].GetAngle();
			error_pos_l[i] = error_pos[i];
			error_pos[i] = pos_d[i] - pos[i];
			kp_c[i] = kp * error_pos[i];
			kd_c[i] = kd * (error_pos[i] - error_pos_l[i]) / dt;
			ki_c[i] = ki_c[i] + ki * error_pos[i] * dt;
			output[i] = kp_c[i] + kd_c[i] + ki_c[i];

//			NANOTECS[i].SetTorque(output[i]);
			switch (i) {
			case 0:
				motor1->SetTorque(output[i]/10.0f);
//				motor1->SetTorque(0.0f);
				break;
			case 1:
				motor2->SetTorque(output[i]/10.0f);
//				motor2->SetTorque(0.0f);
				break;
			case 2:
				motor3->SetTorque(output[i]/10.0f);
//				motor3->SetTorque(0.0f);
				break;
			}
		}
/////		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
	}
}

//void Reboot_Callback(void * param, const std::vector<uint8_t>& payload)
//{
//	// ToDo: Need to check for magic key
//	NVIC_SystemReset();
//}
//
//void EnterBootloader_Callback(void * param, const std::vector<uint8_t>& payload)
//{
//	// ToDo: Need to check for magic key
//	USBD_Stop(&USBCDC::hUsbDeviceFS);
//	USBD_DeInit(&USBCDC::hUsbDeviceFS);
//	Enter_DFU_Bootloader();
//}
