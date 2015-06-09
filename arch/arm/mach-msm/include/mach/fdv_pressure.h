/* =======================================================================
 *
 * when        	who         	why                           		comment tag
 *
 * ----------	---------	-------------------------------------	--------------------------
 *
 *  2014-09-19	yaming.chen	add fdv info for bmp280 			FEIXUN_ADD_FDV_INFO_CHENYAMING_001
 *
 *
 */
#ifndef FDV_PRESSURE_H
#define FDV_PRESSURE_H
#include <asm/types.h>

/*define device_name*/
//#define DEV_TP			"FDV_PRESSURE_H"

/*definition:
	e:	0x0  1  1  0  0000
	bits:0-15:devices id
	bits:16-19:reserved
	bits:20-23:manuf id
	bits:24-27: DEV id

	ex: LCD DEV id is 0, NAND DEV id is 1
	ex: TP DEV id is 2, P/L DEV id is 3*/

/*define manuf_name and id*/
/*PRESSURE*/
#define MANUF_BMP                "BMP"
#define MANUF_BMP_280_ID             0x08000000

#endif

