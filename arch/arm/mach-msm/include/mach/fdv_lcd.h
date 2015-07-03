#ifndef FDV_LCD_H
#define FDV_LCD_H
#include <asm/types.h>


/*define device_name*/
//#define DEV_LCD			"LCD"


/*definition:
	e:	0x0  1  1  0  0000
	bits:0-15:devices id
	bits:16-19:reserved
	bits:20-23:manuf id
	bits:24-27: DEV id

	ex: LCD DEV id is 0, NAND DEV id is 1
	ex: TP DEV id is 2, P/L DEV id is 3 */
         
/*define manuf_name and id*/
/*LCD*/
#define MANUF_FEIXUN		"feixun"
#define MANUF_FEIXUN_ID		0x00100000

#define MANUF_APEX	    	"apex"
#define MANUF_APEX_ID		0x00200000

#define MANUF_BOOYI		"booyi"
#define MANUF_BOOYI_ID		0x00300000

#define MANUF_XINLI		"xinli"
#define MANUF_XINLI_ID		0x00400000

#define MANUF_GUOXIAN		"guoxian"
#define MANUF_GUOXIAN_ID	0x00500000

#define MANUF_BYD               "BYD"
#define MANUF_BYD_ID          	0x00600000

#define MANUF_TRULY             "TRULY"
#define MANUF_TRULY_ID          0x00700000

#define MANUF_JUNDA             "JUNDA"
#define MANUF_JUNDA_ID          0x00800000

#define X130V_BYD_LCD_PN        ToString(810000869) 
#define X130V_FEIXUN_LCD_PN     ToString(907000089) 

#endif
