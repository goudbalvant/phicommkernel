#ifndef FDV_TP_H
#define FDV_TP_H
#include <asm/types.h>

/*define device_name*/
//#define DEV_TP			"TP"

/*definition:
	e:	0x0  1  1  0  0000
	bits:0-15:devices id
	bits:16-19:reserved
	bits:20-23:manuf id
	bits:24-27: DEV id

	ex: LCD DEV id is 0, NAND DEV id is 1
	ex: TP DEV id is 2, P/L DEV id is 3*/
         
/*define manuf_name and id*/
/*TP*/
#define MANUF_FT                "FocalTech"
#define MANUF_FT_ID             0x02100000

#define MANUF_ILITEK            "Ilitek"
#define MANUF_ILITEK_ID         0x02200000

#define MANUF_SITRONIX          "Sitronix"
#define MANUF_SITRONIX_ID       0x02300000

#define MANUF_GOODIX            "Goodix"
#define MANUF_GOODIX_ID_BYD     0x02400000
#define MANUF_GOODIX_ID_FEIXIAN 0X02410000
#define MANUF_GOODIX_ID_JUNDA   0X02420000
#define MANUF_GOODIX_ID_TRULY   0X02430000
#define MANUF_GOODIX_ID_BOYI     0X02440000

#define FT5306_TS               "ft5306_ts"
#define FT5306_TS_ID            0x02500000

#define MANUF_MSG2133           "msg2133"
#define MSG2133_TS_ID           0x02600000

#define MANUF_MSG2138A          "msg2138a"
#define MSG2138A_TS_ID          0x02700000

#define FT5316_TS               "ft5316_ts"
#define FT5316_TS_ID            0x02800000

#define FT6336_TS               "ft6336_ts"  
#define FT6336_TS_ID            0x02900000 

#define FT5336_TS               "ft5336_ts"  
#define FT5336_TS_ID            0x02A00000 

#define MANUF_MSTAR				"mstar"
#define MANUF_MSTAR_ID_JUNDA	0x02A00000

#define X130V_BYD_TP_PN         ToString(810000869) 
#define X130V_FEIXUN_TP_PN      ToString(907000089) 

#endif
