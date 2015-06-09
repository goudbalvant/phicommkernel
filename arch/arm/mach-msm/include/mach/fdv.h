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
#ifndef FREECOMM_DEVICE_VERSION
#define FREECOMM_DEVICE_VERSION
#include <asm/types.h>

#define ToString(x)	#x

#include <mach/fdv_camera.h>
#include <mach/fdv_lcd.h>
#include <mach/fdv_tp.h>
#include <mach/fdv_alsprx.h>
#include <mach/fdv_gsensor.h>
#include <mach/fdv_nand.h>
#include <mach/fdv_emmc.h>
#include <mach/fdv_gyro.h>
#include <mach/fdv_geom.h>
// FEIXUN_ADD_FDV_INFO_CHENYAMING_001 START
#include <mach/fdv_pressure.h>
// FEIXUN_ADD_FDV_INFO_CHENYAMING_001 END

#define FDV_SETUP(_dev)				\
int __init _dev##_setup(char *arg)		\
{												\
	strcpy(ppfi[ppfi_index].name, ToString(_dev));	\
	ppfi[ppfi_index].id = memparse(arg, NULL);	\
	strcpy(ppfi[ppfi_index].part_number, arg);	\
	ppfi_index++;								\
												\
	return 1;									\
}												\
__setup(ToString(_dev=), _dev##_setup);	
	
/*define device_name
#define DEV_LCD			"LCD"
#define DEV_TP			"TP"
#define DEV_NAND		"NAND"
#define DEV_CAMERA		"Camera"
#define DEV_L_P			"L/Psensor"		//light and proximity sensor
#define DEV_G_SENSOR	        "G-sensor" 
#define DEV_GYROSCOPE           "Gyroscope"
#define DEV_GEOMAGNETIC         "Geomagnetic"
*/

/*define device_name*/
#define DEV_CAMERA			ToString(Camera_main)
#define DEV_CAMERA_SECOND	ToString(Camera_second)
#define DEV_L_P				ToString(L_Psensor)
#define DEV_GEOMAGNETIC     ToString(Geomagnetic)
#define DEV_G_SENSOR		ToString(G_sensor)
#define DEV_GYROSCOPE       ToString(Gyroscope)
#define DEV_LCD				ToString(LCD)
#define DEV_NAND			ToString(NAND)
#define DEV_EMMC            ToString(EMMC)
#define DEV_TP				ToString(TP)
// FEIXUN_ADD_FDV_INFO_CHENYAMING_001 START
#define DEV_PRESSURE            ToString(PRESSURE)
// FEIXUN_ADD_FDV_INFO_CHENYAMING_001 END

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


/*Demo
register_fdv(DEV_LCD, MANUF_APEX, MANUF_APEX_ID|lcd_id);
confirm_fdv(DEV_LCD, MANUF_APEX, MANUF_APEX_ID|lcd_id);*/

int register_fdv(char *device_name, char *manuf_name, u32 id);

//the description must LESS than 31 bytes
int register_fdv_with_partno(char *device_name, char *manuf_name, u32 id, char *partno, char *desc);
int register_fdv_with_desc(char *device_name, char *manuf_name, u32 id, char *desc);
int confirm_fdv(char *device_name,char *manuf_name,u32 id);
int confirm_fdv_partno (char *device_name, char *manuf_name, char *partno);

unsigned int get_fdv_id_by_name(char *devname);
unsigned char *get_fdv_part_number_by_name(char *devname);
int fdv_pn_mach(char *devname, char *partno);
#endif
