#ifndef FDV_ALSPRX_H
#define FDV_ALSPRX_H
#include <asm/types.h>

/*define device_name*/
//#define DEV_L_P			"L/Psensor"		/*light and proximity sensor*/

/*definition:
	e:	0x0  1  1  0  0000
	bits:0-15:devices id
	bits:16-19:reserved
	bits:20-23:manuf id
	bits:24-27: DEV id

	ex: LCD DEV id is 0, NAND DEV id is 1
	ex: TP DEV id is 2, P/L DEV id is 3 */
         
/*define manuf_name and id*/

/*L/Psensor*/
#define MANUF_INTERSIL      "Intersil"
#define MANUF_INTERSIL_ID   0x03100000

#define MANUF_TAOS          "Taos"
#define MANUF_TAOS_ID       0x03200000

#endif
