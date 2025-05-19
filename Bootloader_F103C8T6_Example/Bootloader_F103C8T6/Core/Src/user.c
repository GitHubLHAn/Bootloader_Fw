/*
 * USER.c 
 * Created on: 7-MAY-2025
 * Author: Le Huu An
 * Version: 1
 */
 
	#include "user.h"

	#include <string.h>

/*
NOTE: 
	
*/
/**********************************************************************************************************************************/

/*Extern*/

/*Declare struct variable*/

	boot_sys_s vBoot;
	
	volatile uint8_t flag_com_rx = 0;
	
	uint8_t Identify = 1;
	
	UART_HandleTypeDef *pUART;
	
	uint8_t RX_Buffer[NUM_BYTE_BUFFER];
	uint8_t TX_Buffer[NUM_BYTE_BUFFER];
	
	volatile uint16_t size_rx = 0;
	
	uint8_t MODE_FLASH = NOT_FLASH;
	
	uint16_t address_expand = 0x0000;
	uint32_t address_start_flash = 0x0000;
	uint32_t address_end_flash = 0x0000;
	uint16_t size_program_flash = 0x0000;
	
	uint8_t page_start_flash = 0;
	uint8_t page_end_flash = 0;
	
	uint8_t number_page_flash = 0;
	
	
	
/*Declare function*/

//================================================================================
// @ Erase the flash
// @ Erase 1 page
//================================================================================
	uint8_t Flash_Erase(uint32_t address, uint8_t NumPagesErase)
	{
		uint8_t result;
		HAL_FLASH_Unlock();
		FLASH_EraseInitTypeDef EraseInitStruct;
		EraseInitStruct.Banks = 1;
		EraseInitStruct.NbPages = NumPagesErase;
		EraseInitStruct.PageAddress = address;
		EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
		uint32_t page_err;
		result = HAL_FLASHEx_Erase(&EraseInitStruct, &page_err);
		HAL_FLASH_Lock();
		return result;
	}

//================================================================================
// @ Write Array
//================================================================================
	void Flash_Write_Array(uint32_t address, uint8_t *arr, uint16_t len)
	{
		uint16_t *pt = (uint16_t*)arr;
		HAL_FLASH_Unlock();
		for(uint8_t i=0; i<(len+1)/2; i++)
		{
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address + 2*i, *pt);
			pt++;
		}
		HAL_FLASH_Lock();
	}
	
//================================================================================
// @ Write Struct
//================================================================================
	void Flash_Write_Struct(uint32_t address, boot_sys_s pDATA)
	{
		Flash_Write_Array(address, (uint8_t*)&pDATA, sizeof(pDATA));	
	}
	
//================================================================================
// @ Read Array
//================================================================================
	void Flash_Read_Array(uint32_t address, uint8_t *arr, uint16_t len)
	{
		uint16_t *pt = (uint16_t*)arr;
		for(uint8_t i=0; i<(len+1)/2; i++)
		{
			*pt = *(__IO uint16_t*)(address + 2*i);
			pt++;
		}

	}

//================================================================================
// @ Read Struct
//================================================================================
	void Flash_Read_Struct(uint32_t address, boot_sys_s *pDATA)
	{
		Flash_Read_Array(address, (uint8_t*)pDATA, sizeof(boot_sys_s));
	}
	
//================================================================================
// @  
//================================================================================	
	void MODE_RX(void)
	{
			
		HAL_UARTEx_ReceiveToIdle_DMA(pUART, RX_Buffer, NUM_BYTE_BUFFER);
		flag_com_rx = 0;
	}
//================================================================================
// @  
//================================================================================	
	void MODE_TX(void)
	{
	
	}
	

//================================================================================
// @  
//================================================================================	
	
	uint8_t CRC8(uint8_t *data, uint8_t length) 
	{
    uint8_t crc = 0;
		
    for(uint8_t i = 0; i < length - 1; i++) 
		{
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) 
				{
            crc = (crc << 1) ^ (crc & 0x80 ? 0x07 : 0);
        }
    }

		return crc;
	}

//================================================================================
// @  
//================================================================================	
	void Communication_Init(UART_HandleTypeDef *huart)
	{
		flag_com_rx = 0;
		
		pUART = huart;
		HAL_UART_DeInit(huart);
		huart->Init.BaudRate = 115200;
		HAL_UART_Init(huart);
		
		
		for(uint8_t i = 0; i<NUM_BYTE_BUFFER; i++)
		{
			RX_Buffer[i] = 0;
			TX_Buffer[i] = 0;
		}
		
		MODE_RX();
	}
	
	
	
//================================================================================
// @  
//================================================================================	
	
	
	
//================================================================================
// @  
//================================================================================	
	void Handle_Mess_Rx(void)
	{
		if(flag_com_rx == 0) return;
		
		uint8_t result = FAIL;
		
		uint8_t header = RX_Buffer[0];
		
		if(header != Identify){
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			MODE_RX();
			return;
		}
		
		uint8_t length_mess = RX_Buffer[1];
		
		uint8_t check_crc8 = CRC8(RX_Buffer, length_mess);
		
		if(check_crc8 != RX_Buffer[length_mess-1]){
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			MODE_RX();
			return;
		}
		
		uint8_t cmd = RX_Buffer[2];
		
		TOOGLEPIN_LED_DEBUG();
		
		/*
			00: check connect
			01: erase pages
			02: write into flash
		
		*/
		
		if(cmd == CMD_CONNECT){
			MODE_TX();
			
			HAL_UART_Transmit(pUART, RX_Buffer, length_mess, HAL_MAX_DELAY);
			
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			MODE_RX();
			return;
		}
		
		if(cmd == CMD_ERASE_FLASH){
			
			result = SUCCESS;
			
			address_expand = RX_Buffer[3] << 8 | RX_Buffer[4];
			address_start_flash = (address_expand<<16) | (RX_Buffer[5] << 8 | RX_Buffer[6]);
			address_end_flash = (address_expand<<16) | RX_Buffer[7] << 8 | RX_Buffer[8];
			size_program_flash = RX_Buffer[9] << 8 | RX_Buffer[10];			//bytes
			
			page_start_flash = address_start_flash / 0x400;
			
			number_page_flash  = size_program_flash/0x400;
			
			if(size_program_flash%0x400 != 0) 
				number_page_flash++;
			
			// ***** kiem tra cac dieu kien ve kich thuoc
		
			/*Xoa flash*/
			if(Flash_Erase(address_start_flash, number_page_flash) == HAL_OK)
				result = SUCCESS;
			else
				result = FAIL;
			
			if(result == SUCCESS){
				MODE_FLASH = FLASHING;
				vBoot.mode = BOOT_INTO_BOOTLOADER;
				
				Flash_Erase(ADDRESS_Infor, 1);		
				for(uint32_t inx = 10000; inx>0; inx --);			// just for delay
				Flash_Write_Struct(ADDRESS_Infor, vBoot);
			}
			
			MODE_TX();
			
			TX_Buffer[0] = RX_Buffer[0];
			TX_Buffer[1] = 6;
			TX_Buffer[2] = RX_Buffer[2];		
			TX_Buffer[3] = result;
			TX_Buffer[4] = number_page_flash;
			TX_Buffer[5] = CRC8(TX_Buffer, TX_Buffer[1]);
				
			HAL_UART_Transmit(pUART, TX_Buffer, TX_Buffer[1], HAL_MAX_DELAY);
			
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			MODE_RX();
			return;
		}
		
		if(cmd == CMD_FLASHING){
			if(MODE_FLASH != FLASHING){
				MODE_RX();
				return;
			}
			
			result = SUCCESS;
			
			uint32_t address_flash = (address_expand << 16) | ((RX_Buffer[3] << 8) | RX_Buffer[4]);
			
			uint8_t size_flash = RX_Buffer[5];
			uint8_t arr_flash[20];
			
			for(uint8_t cnt=0; cnt<size_flash; cnt++)
				arr_flash[cnt] = RX_Buffer[cnt+6];
			
			uint8_t arr_flash_check[20];
			
//*****check data xem bi trung khong
			Flash_Read_Array(address_flash, arr_flash_check, size_flash);
			
			if(memcmp(arr_flash,arr_flash_check,size_flash) == 0)
			{
				result = SUCCESS;
			}
			else{
				Flash_Write_Array(address_flash, arr_flash, size_flash);
				
				Flash_Read_Array(address_flash, arr_flash_check, size_flash);
			
				if(memcmp(arr_flash,arr_flash_check,size_flash) == 0){
					result = SUCCESS;
				}
				else{
					result = FAIL;
				}
			}
			
			MODE_TX();
				
			TX_Buffer[0] = RX_Buffer[0];
			TX_Buffer[1] = 6;
			TX_Buffer[2] = RX_Buffer[2];		
			TX_Buffer[3] = result;
			TX_Buffer[4] = RX_Buffer[5];
			TX_Buffer[5] = CRC8(TX_Buffer, TX_Buffer[1]);
				
			HAL_UART_Transmit(pUART, TX_Buffer, TX_Buffer[1], HAL_MAX_DELAY);
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			MODE_RX();
			return;
		}
		
		if(cmd == CMD_CONFIRM_DATA){
			
			
			
			
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			MODE_RX();
			return;
		}
		
		if(cmd == CMD_GOTO_APP){
			
			result = SUCCESS;
			
			uint32_t addr_vtor = ((RX_Buffer[3] << 8 | RX_Buffer[4])<<16) | (RX_Buffer[5] << 8 | RX_Buffer[6]);
			
			if(addr_vtor == address_start_flash)
				result = SUCCESS;
			else
				result = FAIL;
			
			if(result == SUCCESS){
				vBoot.mode = BOOT_INTO_APPLICATION;
				vBoot.ver_app = RX_Buffer[7];
				vBoot.VTOR = address_start_flash;
				vBoot.day = RX_Buffer[8];
				vBoot.month = RX_Buffer[9];
				vBoot.year = (RX_Buffer[10]<<8) | RX_Buffer[11];
				
				Flash_Erase(ADDRESS_Infor, 1);		
				for(uint32_t inx = 10000; inx>0; inx --);			// just for delay
				Flash_Write_Struct(ADDRESS_Infor, vBoot);
			}
			
			MODE_TX();
			
			TX_Buffer[0] = RX_Buffer[0];
			TX_Buffer[1] = 5;
			TX_Buffer[2] = RX_Buffer[2];		
			TX_Buffer[3] = result;
			TX_Buffer[4] = CRC8(TX_Buffer, TX_Buffer[1]);
				
			HAL_UART_Transmit(pUART, TX_Buffer, TX_Buffer[1], HAL_MAX_DELAY);

			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			MODE_RX();
			
			
			NVIC_SystemReset();
			return;
		}
		
	}
	

//================================================================================
// @  
//================================================================================
	volatile uint16_t cnt_rx = 0;
	void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) 
	{
		UNUSED(huart);
		UNUSED(Size);
		
		if(huart->Instance == pUART->Instance) 
		{	
			cnt_rx++;
			flag_com_rx = 1;
			size_rx	= Size;		
		}
	}


//================================================================================
// @  
//================================================================================	
	

/**********************************************************************************************************************************/