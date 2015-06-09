#ifndef FDV_NAND_H
#define FDV_NAND_H
#include <asm/types.h>

/*define device_name*/
//#define DEV_NAND		"NAND"

/*definition:
	e:	0x0  1  1  0  0000
	bits:0-15:devices id
	bits:16-19:reserved
	bits:20-23:manuf id
	bits:24-27: DEV id

	ex: LCD DEV id is 0, NAND DEV id is 1
	ex: TP DEV id is 2, P/L DEV id is 3 */
         
/*define manuf_name and id*/
/*NAND*/
#define MANUF_HYNIX	    	"Hynix"
#define MANUF_HYNIX_ID		0x01100000

#define MANUF_MICRON		"Micron"
#define MANUF_MICRON_ID		0x01200000

#define MANUF_SAMSUNG		"Samsung"
#define MANUF_SAMSUNG_ID	0x01300000

#define MANUF_TOSHIBA		"Toshiba"
#define MANUF_TOSHIBA_ID	0x01400000

#endif
