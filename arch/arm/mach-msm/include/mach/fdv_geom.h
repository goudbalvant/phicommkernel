#ifndef FDV_GEOM_H
#define FDV_GEOM_H
#include <asm/types.h>

/*define device_name*/
//#define DEV_GEOMAGNETIC         "Geomagnetic"

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

/*Geomagnetic*/

#define MANUF_MEMSIC            "MEMSIC"
#define MANUF_MEMSIC_ID         0x07100000

#define MANUF_AKM            	"AKM"
#define MANUF_AKM_ID         	0x07200000

#endif
