#ifndef FDV_GYRO_H
#define FDV_GYRO_H
#include <asm/types.h>

/*define device_name*/
//#define DEV_GYROSCOPE           "Gyroscope"

/*definition:
	e:	0x0  1  1  0  0000
	bits:0-15:devices id
	bits:16-19:reserved
	bits:20-23:manuf id
	bits:24-27: DEV id

	ex: LCD DEV id is 0, NAND DEV id is 1
	ex: TP DEV id is 2, P/L DEV id is 3
*/        
/*define manuf_name and id*/

/*Gyroscope*/
#define MANUF_RICHPOWER        "RichPower"
#define MANUF_RICHPOWER_ID     0x06100000

#endif
