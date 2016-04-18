#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
//#define _DEBUG_ 1
#ifdef _DEBUG_
#define TRACE(fmt,a...) fprintf(stderr,"[%s][%s][%03d]:"fmt"\n",__FILE__,__FUNCTION__,__LINE__,##a)
#else
#define TRACE(fmt,a...)
#endif

static int rtc_hour, rtc_min, rtc_set;
static const char default_rtc[] = "/dev/rtc0";
static const char config_rtc[] = "/etc/rtc_configure";

static int get_start_time(int hour, int min){
	 size_t now, now_hour, now_min, write_seconds;
	 struct tm* _tm;
	 now = time(0);
	 _tm = localtime(&now);
	 now_hour = _tm->tm_hour;
	 now_min = _tm->tm_min;
	 TRACE("now_hour = %d\n", _tm->tm_hour);
	 TRACE("now_min = %d\n", _tm->tm_min);
	 if((now_hour > hour) | ((now_hour == hour) &&(_tm->tm_min > min)) )
	     write_seconds = (24 - now_hour + hour) * 60 * 60 + (min - now_min) * 60;
	 else
	     write_seconds = (hour - now_hour) * 60 * 60 + (min - now_min) * 60;
	 return write_seconds;
}

static int issetclock(const char* path){
	char buffer[512] = {0};
	char *t = NULL;
	int lines = 1;
	FILE* fp = fopen(path, "r");
	if(!fp)
	{
		TRACE("fopen");
		return 0;
	}
	while(!feof(fp)){
		if(fgets(buffer, sizeof(buffer), fp) != NULL){
			t = strrchr(buffer, '=');
			t++;
			while(isspace(*t))
				t++;
			if(lines == 1){
				rtc_set = atoi(t);
				TRACE("rtc_set = %d", rtc_set);
				if(!rtc_set)
					return 0;
			}
			if(lines == 2){
				rtc_hour = atoi(t);
				TRACE("rtc_hour = %d", rtc_hour);
			}
			if(lines == 3){
				rtc_min = atoi(t);
				TRACE("rtc_min = %d", rtc_min);
			}
			lines++;
		}
	}
	return 1;
}

int main(int argc, char **argv)
{
    int i, fd, retval, irqcount = 0, alarm_time = 5;
    unsigned long tmp, data;
	
    struct rtc_time rtc_tm;
    const char *rtc = default_rtc;
	const char *configure = config_rtc;
	fd = open(rtc, O_RDONLY);
    if (fd ==  -1)
    {
        TRACE("open");
        exit(errno);
    }
	if(!issetclock(configure))
		goto SET_RTC_OFF;
    TRACE("\n\t\t\tRTC Driver Test Example.\n\n");

    /* Read the RTC time/date */

   TRACE("rtc_hour = %d, rtc_min = %d", rtc_hour, rtc_min);
   alarm_time = get_start_time(rtc_hour, rtc_min);
    TRACE("%d seconds", alarm_time);

    retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
    if (retval == -1)
    {
        TRACE("RTC_RD_TIME ioctl");
        exit(errno);
    }
    TRACE("\n\nCurrent RTC date\time is %d-%d-%d, %02d:%02d:%02d.\n",rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
	rtc_tm.tm_min += (alarm_time%(60*60))/60;
	if(rtc_tm.tm_min >= 60){
		rtc_tm.tm_min -= 60;
		rtc_tm.tm_hour++;
	}	
	rtc_tm.tm_hour += (alarm_time/(60*60));
	if(rtc_tm.tm_hour >= 24){
		rtc_tm.tm_hour -= 24;
	}
	
    TRACE("Alarm time now set to %02d:%02d:%02d.\n",rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
	#if 1
    retval = ioctl(fd, RTC_ALM_SET, &rtc_tm);
    if (retval == -1)
    {
        if (errno == ENOTTY)
        {
            fprintf(stderr,"\n...Alarm IRQs not supported.\n");
            //goto test_PIE;
        }
        TRACE("RTC_ALM_SET ioctl");
        exit(errno);
    }
    /* Read the current alarm settings */
    retval = ioctl(fd, RTC_ALM_READ, &rtc_tm);
    if (retval == -1)
    {
        TRACE("RTC_ALM_READ ioctl");
        exit(errno);
    }
    
    fflush(stderr);
    /* Enable alarm interrupts */
    retval = ioctl(fd, RTC_AIE_ON, 0);
    if (retval == -1)
    {
        TRACE("RTC_AIE_ON ioctl");
        exit(errno);
    }

    /* Read the current alarm settings */
    retval = ioctl(fd, RTC_ALM_READ, &rtc_tm);
    if (retval == -1)
    {
        TRACE("RTC_ALM_READ ioctl");
        exit(errno);
    }
    //TRACE("Alarm time now set to %02d:%02d:%02d.\n",rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
    fflush(stderr);
    /* Enable alarm interrupts */
    retval = ioctl(fd, RTC_AIE_ON, 0);
    if (retval == -1)
    {
        TRACE("RTC_AIE_ON ioctl");
        exit(errno);
    }
	#endif
    TRACE("\t *** Test complete ***\n");
    close(fd);
	return 0;
	SET_RTC_OFF:
	{
		retval = ioctl(fd, RTC_AIE_OFF, 0);
	    if (retval == -1)
	    {
	        TRACE("RTC_AIE_OFF ioctl");
	        exit(errno);
	    }
	    TRACE("\t *** Test complete ***\n");
	}
    return 0;
}

