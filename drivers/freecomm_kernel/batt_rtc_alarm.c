#include <linux/module.h>
#include <linux/android_alarm.h>
#include <linux/wakelock.h>

#define BATT_RTC_TIME_S			1800
#define BATT_RTC_LOCK_TIME_S	22

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yatang.li<yatang.li@freecomm.com.cn>");
MODULE_DESCRIPTION("Freecomm False rtc alarm ervey 30min for battery");

struct wake_lock batt_wake_lock;
struct alarm batt_alarm_struct;

static void batt_alarm_triggered(struct alarm *alarm)
{
	struct timespec batt_tmp_time;

	printk(KERN_INFO "BATT_RTC_ALARM\t%s\t%d\n",__func__,__LINE__);
	
	/*lock the wake lock*/
	wake_lock(&batt_wake_lock);
	/*set after BATT_RTC_LOCK_TIME_S s the wake lock open*/
	wake_lock_timeout(&batt_wake_lock,BATT_RTC_LOCK_TIME_S*HZ);
	
	/*set next rtc alarm time after BATT_RTC_TIME_S s*/
	getnstimeofday(&batt_tmp_time);
	batt_tmp_time.tv_sec += BATT_RTC_TIME_S;
	alarm_start_range(&batt_alarm_struct,timespec_to_ktime(batt_tmp_time),timespec_to_ktime(batt_tmp_time));
}

static int __init batt_rtc_alarm_init(void)
{
	/*init a wake lock*/
	wake_lock_init(&batt_wake_lock, WAKE_LOCK_SUSPEND, "batt");
	
	/*init rtc alarm*/
	alarm_init(&batt_alarm_struct, 0, batt_alarm_triggered);
	/*first set rtc alarm*/
	batt_alarm_triggered(&batt_alarm_struct);

	return 1;
}

static void __exit batt_rtc_alarm_exit(void)
{
	wake_lock_destroy(&batt_wake_lock);
}

module_init(batt_rtc_alarm_init);
module_exit(batt_rtc_alarm_exit);
