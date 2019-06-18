/*
 * This is a every simple sample for MiniGUI.
 * It will create a main window and display a string of "Hello, world!" in it.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> 
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
extern WINDOW_ELEMENT_RENDERER * __mg_def_renderer;
extern MG_EXPORT PLOGFONT g_SysLogFont [];

#include "common.h"
#include "desktop_dialog.h"
#include "hardware.h"

#include <DeviceIo/Rk_wifi.h>

char timebuff[100];
static RECT msg_rcTime = {TIME_PINT_X, TIME_PINT_Y, TIME_PINT_X + TIME_PINT_W, TIME_PINT_Y + TIME_PINT_H};
#ifdef ENABLE_BATT
RECT msg_rcBatt = {BATT_PINT_X, BATT_PINT_Y, BATT_PINT_X + BATT_PINT_W, BATT_PINT_Y + BATT_PINT_H};
#endif
RECT msg_rcStatusBar = {STATUS_BAR_X, STATUS_BAR_Y, STATUS_BAR_X + STATUS_BAR_W, STATUS_BAR_Y + STATUS_BAR_H};

#ifdef ENABLE_WIFI
RECT msg_rcWifi = {WIFI_PINT_X, WIFI_PINT_Y, WIFI_PINT_X + WIFI_PINT_W, WIFI_PINT_Y + WIFI_PINT_H};
#endif

RECT msg_rcBg = {BG_PINT_X, BG_PINT_Y, BG_PINT_X + BG_PINT_W, BG_PINT_Y + BG_PINT_H};
RECT msg_rcTitle = {TITLE_PINT_X, TITLE_PINT_Y, TITLE_PINT_X + TITLE_PINT_W, TITLE_PINT_Y + TITLE_PINT_H};
RECT msg_rcDialog = {0, 0, LCD_W, LCD_H};

#ifdef ENABLE_BATT
BITMAP batt_bmap[6];
#endif
#ifdef ENABLE_WIFI
BITMAP wifi_bmap;
BITMAP wifi_connected_bmap;
BITMAP wifi_disconnected_bmap;
BITMAP wifi_disabled_bmap;
#endif

BITMAP back_bmap;
BITMAP background_bmap;

BITMAP volume_0;
BITMAP volume_1;
BITMAP volume_2;
BITMAP volume_3;
BITMAP airkiss_bmap;
BITMAP list_sel1_bmap;
BITMAP wifi_signal_3;
BITMAP wifi_signal_2;
BITMAP wifi_signal_1;
BITMAP input_box;

struct tm *now_time = 0;
struct tm *last_time = 0;
rtc_timing timing_power_on[TIMING_NUM+1] = {0};
rtc_timing timing_power_off[TIMING_NUM+1] = {0};
char *timing_buf;
int power_off_time;
int power_on_time;
int systemtime_year = 2019;
int systemtime_month = 5;
int systemtime_day = 30;
int time_hour = 0;
int time_min = 0;
int time_sec = 0;
int battery = 0;
int status_bar_offset;
int status_bar_time_str[10] = {0};
int status_bar_date_str[20] = {0};

char *res_str[RES_STR_MAX] = {0};
LOGFONT  *logfont_cej;
LOGFONT  *logfont_k;
LOGFONT  *logfont_cej_title;
LOGFONT  *logfont_k_title;
LOGFONT  *logfont;
LOGFONT  *logfont_title;

static int screenoff_cnt = 0;
static int screenautooff = 1;
HWND mhWnd;

#define maxlabelsize 35
static int  __getline(char **lineptr, ssize_t *n, FILE *stream)
{
    int count=0;
    int buf;

    if(*lineptr == NULL) {
        *n=maxlabelsize;
        *lineptr = (char*)malloc(*n);
    }

    if(( buf = fgetc(stream) ) == EOF ) {
        return -1;
    }

    do {
        if(buf=='\n') {
            count += 1;
            break;
        }

        count++;

        *(*lineptr+count-1) = buf;
        *(*lineptr+count) = '\0';

        if(*n <= count)
            *lineptr = (char*)realloc(*lineptr,count*2);
        buf = fgetc(stream);
    } while( buf != EOF);

    return count;
}

void updatesysfont(LOGFONT  *font)
{
    int i;

    for (i = 0; i <= WE_DESKTOP; i++) {
        __mg_def_renderer->we_fonts[i] = font;
    }
    for (i = 0; i < NR_SYSLOGFONTS; i++) {
        g_SysLogFont[i] = font;
    }
}

int loadversion(char **model, char **version)
{
    FILE *fp;
    ssize_t len = 0;
    int pos = 0;
    const char* versionFile;

    versionFile = VERSION_FILE;

    fp = fopen(versionFile, "r");
    if (fp == NULL) {
        printf("open file %s failed: %s\n", versionFile, strerror(errno));
        return -1;
    }

    if ((__getline(model, &len, fp)) == -1)
        return -1;

    if ((__getline(version, &len, fp)) == -1)
        return -1;

    fclose(fp);

    return 0;	
}

int loadtimefile(void)
{
    FILE *fp;
    ssize_t len = 0;
    int pos = 0;
    use_24_hour_format = 1;
    status_bar_offset = 0;
    sync_net_time = 0;
    fp = fopen(TIMING_FILE, "r");
    if (fp == NULL) {
        printf("open file %s failed: %s\n", TIMING_FILE, strerror(errno));
        return -1;
    }
    while ((__getline(&timing_buf, &len, fp)) != -1) {
        timing_power_on[pos].timing = ((*timing_buf - 48) * 1000) + ((*(timing_buf+1) - 48) * 100)
                                    + ((*(timing_buf+2) - 48) * 10) + (*(timing_buf+3) - 48);
        timing_power_on[pos].status = (*(timing_buf+4) - 48);
        pos++;
        if (pos >= TIMING_NUM)
            break;
    }
    pos = 0;
    while ((__getline(&timing_buf, &len, fp)) != -1) {
        timing_power_off[pos].timing = ((*timing_buf - 48) * 1000) + ((*(timing_buf+1) - 48) * 100)
                                    + ((*(timing_buf+2) - 48) * 10) + (*(timing_buf+3) - 48);
        timing_power_off[pos].status = (*(timing_buf+4) - 48);
        pos++;
        if (pos >= TIMING_NUM)
            break;
    }

    fclose(fp);

    fp = fopen(TIME_SETTING_FILE, "r");
    if (fp == NULL) {
        printf("open file %s failed: %s\n", TIME_SETTING_FILE, strerror(errno));
        free(timing_buf);
        timing_buf = 0;
        return -1;
    }
    if ((__getline(&timing_buf, &len, fp)) != -1) {
        use_24_hour_format = *timing_buf - 48;
        if (!use_24_hour_format)
            status_bar_offset = STATUS_BAR_ICO_OFFSET;
        sync_net_time = *(timing_buf+1) - 48;
    }
    free(timing_buf);
    timing_buf = 0;
    fclose(fp);

}

int loadstringres(void)
{
    FILE *fp;
    ssize_t len = 0;
    int pos = 0;
    const char* langFile;

    switch (get_language()) {
        case LANGUAGE_CH:
            langFile = REC_FILE_CN;
            logfont = logfont_cej;
		    logfont_title = logfont_cej_title;
            break;
        case LANGUAGE_EN:
            langFile = REC_FILE_EN;
            logfont = logfont_cej;
			logfont_title = logfont_cej_title;
            break;
        case LANGUAGE_JA:
            langFile = REC_FILE_JA;
            logfont = logfont_cej;
			logfont_title = logfont_cej_title;
            break;
        case LANGUAGE_KO:
            langFile = REC_FILE_KO;
            logfont = logfont_k;
			logfont_title = logfont_k_title;
            break;
    }
    fp = fopen(langFile, "r");
    if (fp == NULL) {
        printf("open file %s failed: %s\n", langFile, strerror(errno));
        return -1;
    }

    fgetc(fp);
    fgetc(fp);
    fgetc(fp);

    while ((__getline(&res_str[pos], &len, fp)) != -1) {
        //printf("load line Label %d------%s\n", pos, res_str[pos]);
        pos++;
        if (pos >= RES_STR_MAX)
            break;
    }

    fclose(fp);
    updatesysfont(logfont);
}

void unloadstringres(void)
{
    int i;

    for (i = 0; i < RES_STR_MAX; i++) {
        if (res_str[i]) {
            free(res_str[i]);
            res_str[i] = 0;
        }
    }
}

static char* mk_time(char* buff)
{
     time_t t;
     struct tm * tm;

     time (&t);
     tm = localtime (&t);
     sprintf (buff, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

     return buff;
}

void sysUsecTime(char* buff)
{
    struct timeval    tv;
    struct timezone tz;

    struct tm         *p;

    gettimeofday(&tv, &tz);

    p = localtime(&tv.tv_sec);
    sprintf(buff, "%04d-%02d-%02d %02d:%02d:%02d %03d", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec / 1000);
}


#ifdef ENABLE_WIFI

int _RK_wifi_state_callback(RK_WIFI_RUNNING_State_e state)
{
	printf("%s state: %d\n", __func__, state);

	set_wifi(state);
 //   if (get_bl_brightness() == 0)
 //   {
 //       screenon();
 //   }
//	DesktopUpdateAllWindow();

	if (state == RK_WIFI_State_CONNECTED) {
		printf("RK_WIFI_State_CONNECTED\n");
	
	} else if (state == RK_WIFI_State_CONNECTFAILED) {
		printf("RK_WIFI_State_CONNECTFAILED\n");
	} else if (state == RK_WIFI_State_CONNECTFAILED_WRONG_KEY) {
		printf("RK_WIFI_State_CONNECTFAILED_WRONG_KEY\n");
	}
	 else if (state == RK_WIFI_State_CONNECTING) {
		printf("RK_WIFI_State_CONNECTING\n");
		set_wifi_date(wifi_date.ssid,wifi_date.psk);
	}

	return 0;

}

#endif



int main_loadres(void)
{
    int i;
    char img[128];
    char *respath = get_ui_image_path();

#ifdef ENABLE_BATT
    for (i = 0; i < 6; i++) {
        snprintf(img, sizeof(img), "%sbattery%d.png", respath, i);
        if (LoadBitmap(HDC_SCREEN, &batt_bmap[i], img))
            return -1;
    }
#endif

#ifdef ENABLE_WIFI
   snprintf(img, sizeof(img), "%swifi.png", respath);
   if (LoadBitmap(HDC_SCREEN, &wifi_bmap, img)) 
		return -1;

   snprintf(img, sizeof(img), "%swifi_connected.png", respath);
   if (LoadBitmap(HDC_SCREEN, &wifi_connected_bmap, img)) 
   	    return -1;
   
   snprintf(img, sizeof(img), "%swifi_disconnected.png", respath);
   if (LoadBitmap(HDC_SCREEN, &wifi_disconnected_bmap, img)) 
   	    return -1;   
   
   snprintf(img, sizeof(img), "%swifi_disabled.png", respath);
   if (LoadBitmap(HDC_SCREEN, &wifi_disabled_bmap, img)) 
   	    return -1;  

	
#endif

    snprintf(img, sizeof(img), "%sback.png", respath);
    if (LoadBitmap(HDC_SCREEN, &back_bmap, img))
        return -1;

    snprintf(img, sizeof(img), "%sbackground.jpg", respath);
    if (LoadBitmap(HDC_SCREEN, &background_bmap, img))
        return -1;

//=================add vulume icon=======================
	
    snprintf(img, sizeof(img), "%svolume_0.png", respath);
    if (LoadBitmap(HDC_SCREEN, &volume_0, img))
        return -1;
	
	snprintf(img, sizeof(img), "%svolume_1.png", respath);
    if (LoadBitmap(HDC_SCREEN, &volume_1, img))
        return -1;
	
	snprintf(img, sizeof(img), "%svolume_2.png", respath);
    if (LoadBitmap(HDC_SCREEN, &volume_2, img))
        return -1;
	
	snprintf(img, sizeof(img), "%svolume_3.png", respath);
    if (LoadBitmap(HDC_SCREEN, &volume_3, img))
        return -1;


	snprintf(img, sizeof(img), "%sairkiss.jpg", respath);
    if (LoadBitmap(HDC_SCREEN, &airkiss_bmap, img))
        return -1;


	snprintf(img, sizeof(img), "%swifi_signal_3.png", respath);
	if (LoadBitmap(HDC_SCREEN, &wifi_signal_3, img))
		return -1;

	snprintf(img, sizeof(img), "%swifi_signal_2.png", respath);
	if (LoadBitmap(HDC_SCREEN, &wifi_signal_2, img))
		return -1;

	snprintf(img, sizeof(img), "%swifi_signal_1.png", respath);
	if (LoadBitmap(HDC_SCREEN, &wifi_signal_1, img))
		return -1;

	snprintf(img, sizeof(img), "%sinput_box.png", respath);
	if (LoadBitmap(HDC_SCREEN, &input_box, img))
		return -1;

	snprintf(img, sizeof(img), "%slist_sel1.png", respath);
	if (LoadBitmap(HDC_SCREEN, &list_sel1_bmap, img))
		return -1;
	
    return 0;
}

void main_unloadres(void)
{
    int i;

#ifdef ENABLE_BATT
    for (i = 0; i < 6; i++)
      UnloadBitmap(&batt_bmap[i]);
#endif

    UnloadBitmap(&background_bmap);
}

#ifdef ENABLE_BATT
static void batt_update(void)
{
    if (ac_is_online()) {
        battery = 5;
    } else {
        int bat = get_battery_capacity();
        if (bat < 5) {
            battery = 0;
            creat_lowpower_dialog(mhWnd);
        } else if (bat < 10)
            battery = 0;
        else if (bat < 30)
            battery = 1;
        else if (bat < 50)
            battery = 2;
        else if (bat < 80)
            battery = 3;
        else
            battery = 4;
    }
}
#endif

void DisableScreenAutoOff(void)
{
    screenautooff = 0;
    screenoff_cnt = 0;
    EnableKeyMessage();
    screenon();
}

void EnableScreenAutoOff(void)
{
    screenautooff = 1;
    screenoff_cnt = 0;
}

static LRESULT MainWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;

    switch (message) {
        case MSG_CREATE:
            EnableScreenAutoOff();
            main_loadres();
            logfont_cej = CreateLogFont("ttf", "msyh", "UTF-8", 'k', 'r', 'n', 'c', 'n', 'n', TTF_FONT_SIZE, 0);
            logfont_k = CreateLogFont("ttf", "msn", "UTF-8", 'k', 'r', 'n', 'c', 'n', 'n', TTF_FONT_SIZE, 0);
			
			logfont_cej_title = CreateLogFont("ttf", "msyh", "UTF-8", 'k', 'r', 'n', 'c', 'n', 'n', TTF_TITLE_FONT_SIZE, 0);
			logfont_k_title = CreateLogFont("ttf", "msn", "UTF-8", 'k', 'r', 'n', 'c', 'n', 'n', TTF_TITLE_FONT_SIZE, 0);
		
            loadstringres();
            loadtimefile();
            SetTimer(hWnd, _ID_TIMER_MAIN, TIMER_MAIN);


#ifdef ENABLE_BATT
            batt_update();
#endif

#ifdef ENABLE_WIFI
			
				if(get_wifi())
				{
					RK_wifi_enable(1);
					set_wifi(RK_WIFI_State_DISCONNECTED);
					int iret1;
					iret1 = pthread_create( &thread1, NULL, get_available_wifi, (void*) message1);
				//	pthread_detach(&thread1);
					printf("Thread 1 returns: %d\n",iret1);  // return 0 if seccuess
			
				}
				else
				{
					RK_wifi_enable(0);
					set_wifi(RK_WIFI_State_IDLE);
			
				}
			
				
#endif


            InvalidateRect(hWnd, &msg_rcBg, TRUE);
            RegisterMainWindow(hWnd);
            mhWnd = hWnd;
            creat_desktop_dialog(hWnd);
        break;
        case MSG_TIMER:
            if (wParam == _ID_TIMER_MAIN) {
                if (screenautooff && (screenoff_cnt < get_screenoff_val())) {
                    screenoff_cnt ++;
                    if (screenoff_cnt == get_screenoff_val()) {
                        DisableKeyMessage();
                        screenoff();
                    }
                }
		/* for lowpower test
		int test_cnt=0
		test_cnt ++;
		if (test_cnt == 5) {
			battery = 0;
			creat_lowpower_dialog(mhWnd);
		}
		*/
            }
        break;
        case MSG_PAINT:
            hdc = BeginPaint(hWnd);

            FillBoxWithBitmap(hdc, BG_PINT_X,
                              BG_PINT_Y, BG_PINT_W,
                              BG_PINT_H, &background_bmap);
            EndPaint(hWnd, hdc);
            break;
        case MSG_KEYDOWN:
            printf("%s MSG_KEYDOWN message = 0x%x, 0x%x, 0x%x\n", __func__, message, wParam, lParam);
            break;
        case MSG_MAINWIN_KEYDOWN:
            break;
        case MSG_MAINWIN_KEYUP:
            if (screenoff_cnt == get_screenoff_val()) {
                EnableKeyMessage();
                screenon();
            }
            screenoff_cnt = 0;
            break;
        case MSG_CLOSE:
            KillTimer(hWnd, _ID_TIMER_MAIN);
            UnregisterMainWindow(hWnd);
            DestroyMainWindow(hWnd);
            PostQuitMessage(hWnd);
            main_unloadres();
            unloadstringres();
            DestroyLogFont(logfont_cej);
            DestroyLogFont(logfont_k);
        return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static void InitCreateInfo(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle = WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_AUTOSECONDARYDC;
    pCreateInfo->spCaption = "Main" ;
    pCreateInfo->hMenu = 0;
    //pCreateInfo->hCursor = GetSystemCursor (0);
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = MainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = LCD_W;
    pCreateInfo->by = LCD_H;
    pCreateInfo->iBkColor = PIXEL_lightwhite;
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void signal_func(int signal)
{
    switch (signal){
        case SIGUSR1:
#ifdef ENABLE_BATT
            batt_update();
#endif
            break;
        default:
            break;
    }
}

int MiniGUIMain(int args, const char* arg[])
{
    MSG Msg;
    MAINWINCREATE CreateInfo;
    struct sigaction sa;
    FILE *pid_file;
    HWND hMainWnd;

#ifdef _MGRM_PROCESSES
    JoinLayer (NAME_DEF_LAYER, arg[0], 0, 0);
#endif


    parameter_init();
	keyboard_init();
    screenon();
    InitCreateInfo (&CreateInfo);

    pid_file = fopen("/tmp/pid", "w");
    if (!pid_file) {
        printf("open /tmp/pid fail...\n");
        return -1;
    }

    fprintf(pid_file, "%d", getpid());
    fclose(pid_file);

    sa.sa_sigaction = NULL;
    sa.sa_handler   = signal_func;
    sa.sa_flags     = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    hMainWnd = CreateMainWindow (&CreateInfo);
    if (hMainWnd == HWND_INVALID)
        return -1;

    while (GetMessage (&Msg, hMainWnd)) {
        DispatchMessage (&Msg);
    }
    MainWindowThreadCleanup (hMainWnd);
    parameter_deinit();
    return 0;
}
