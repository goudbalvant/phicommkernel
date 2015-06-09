#ifndef FDV_GSENSOR_H
#define FDV_GSENSOR_H
#include <asm/types.h>

/*define device_name*/
//#define DEV_G_SENSOR	"G-sensor"

/*definition:
	e:	0x0  1  1  0  0000
	bits:0-15:devices id
	bits:16-19:reserved
	bits:20-23:manuf id
	bits:24-27: DEV id

	ex: LCD DEV id is 0, NAND DEV id is 1
	ex: TP DEV id is 2, P/L DEV id is 3 */
         
/*define manuf_name and id*/
/*Gsensor*/
#define MANUF_ADI              "ADI"
#define MANUF_ADI_ADXL34x_ID   0x05100000

#define MANUF_KIONIX           "KIONIX"
#define MANUF_KIONIX_KXTIK_ID  0x05200000

#define MANUF_AFA750           "afa750"
#define MANUF_AFA_AFA750_ID    0x05300000

#define MANUF_BOSCH		"bosch"
#define MANUF_BOSCH_BMA222_ID	0x05400000
#endif
