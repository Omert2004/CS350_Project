/*
 * BL_Update_Part.h
 *
 *  Created on: 22 Ara 2025
 *      Author: Oguzm
 */

#ifndef INC_BL_UPDATE_PART_H_
#define INC_BL_UPDATE_PART_H_

void Bootloader_SetStatus(uint32_t status);
void BL_RequestUpdate(void);
uint32_t BL_GetStatus(void);

int Install_Update_Stream(uint8_t is_dry_run);
void Bootloader_HandleUpdate(void);


#endif /* INC_BL_UPDATE_PART_H_ */
