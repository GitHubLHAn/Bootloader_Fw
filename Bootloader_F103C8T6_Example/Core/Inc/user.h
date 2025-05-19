/*
 * USER.h 
 * Created on: 7-MAY-2025
 * Author: Le Huu An
 * Version: 1
 */

#ifndef USER_H_
#define USER_H_

#include <main.h>


/*DEFINE*/

	#define ADDRESS_Infor ((uint32_t)0x08007C00U)
	
//	#define ADDRESS_APP ((uint32_t)0x08002800U)
	
	
	#define BOOT_INTO_BOOTLOADER  0x11
	#define BOOT_INTO_APPLICATION 0x22
	
	#define ON_LED_DEBUG(	)			HAL_GPIO_WritePin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin, GPIO_PIN_RESET);
	#define OFF_LED_DEBUG(	)			HAL_GPIO_WritePin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin, GPIO_PIN_SET);
	
	#define TOOGLEPIN_LED_DEBUG(	)			HAL_GPIO_TogglePin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin);
	
	#define NUM_BYTE_BUFFER 50
	
	#define NOT_FLASH	0
	#define FLASHING	1
	
	#define SUCCESS			0x59
	#define FAIL				0x4E	
	
	#define CMD_CONNECT	0x00
	#define CMD_ERASE_FLASH	0x01
	#define CMD_FLASHING	0x02
	#define CMD_CONFIRM_DATA	0x03
	#define CMD_GOTO_APP	0x04
	


/************************************************************************************/
/*DECLARE STRUCT*/
	#pragma pack(1)
		typedef struct{
			uint8_t mode;
			uint8_t ver_app;
			uint8_t day;
			uint8_t month;
			uint16_t year;
			uint32_t VTOR;
	}boot_sys_s;
	#pragma pack()


/*DECLARE FUNCTION*/
	
	uint8_t Flash_Erase(uint32_t address, uint8_t NumPagesErase);
	
	void Flash_Write_Struct(uint32_t address, boot_sys_s data);
	
	void Flash_Read_Struct(uint32_t address, boot_sys_s *data);	
	
	void Communication_Init(UART_HandleTypeDef *huart);
	
	void Handle_Mess_Rx(void);
	
/*EXTERN*/
	
	extern boot_sys_s vBoot;
	
	extern volatile uint8_t flag_com_rx;
	
	extern UART_HandleTypeDef *pUART;
	
/************************************************************************************/
#endif /* USER_H */

