/*=========================================================================*/
/*  Includes                                                               */
/*=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <system.h>
#include <sys/alt_alarm.h>
#include <io.h>

#include "fatfs.h"
#include "diskio.h"

#include "ff.h"
#include "monitor.h"
#include "uart.h"

#include "alt_types.h"

#include <altera_up_avalon_audio.h>
#include <altera_up_avalon_audio_and_video_config.h>

//#include <unistd.h>

/*=========================================================================*/
/*  DEFINE: All Structures and Common Constants                            */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Macros                                                         */
/*=========================================================================*/

#define PSTR(_a)  _a

/*=========================================================================*/
/*  DEFINE: Prototypes                                                     */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Definition of all local Data                                   */
/*=========================================================================*/
static alt_alarm alarm;
static unsigned long Systick = 0;
static volatile unsigned short Timer;   /* 1000Hz increment timer */

/*=========================================================================*/
/*  DEFINE: Definition of all local Procedures                             */
/*=========================================================================*/

/***************************************************************************/
/*  TimerFunction                                                          */
/*                                                                         */
/*  This timer function will provide a 10ms timer and                      */
/*  call ffs_DiskIOTimerproc.                                              */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static alt_u32 TimerFunction (void *context)
{
   static unsigned short wTimer10ms = 0;

   (void)context;

   Systick++;
   wTimer10ms++;
   Timer++; /* Performance counter for this module */

   if (wTimer10ms == 10)
   {
      wTimer10ms = 0;
      ffs_DiskIOTimerproc();  /* Drive timer procedure of low level disk I/O module */
   }

   return(1);
} /* TimerFunction */

/***************************************************************************/
/*  IoInit                                                                 */
/*                                                                         */
/*  Init the hardware like GPIO, UART, and more...                         */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static void IoInit(void)
{
   uart0_init(115200);

   /* Init diskio interface */
   ffs_DiskIOInit();

   //SetHighSpeed();

   /* Init timer system */
   alt_alarm_start(&alarm, 1, &TimerFunction, NULL);

} /* IoInit */

/*=========================================================================*/
/*  DEFINE: All code exported                                              */
/*=========================================================================*/

uint32_t acc_size;                 /* Work register for fs command */
uint16_t acc_files, acc_dirs;
FILINFO Finfo;
#if _USE_LFN
char Lfname[512];
#endif

char Line[256];                 /* Console input buffer */

FATFS Fatfs[_VOLUMES];          /* File system object for each logical drive */
FIL File1, File2;               /* File objects */
DIR Dir;                        /* Directory object */
uint8_t Buff[8192] __attribute__ ((aligned(4)));  /* Working buffer */




static
FRESULT scan_files(char *path)
{
    DIR dirs;
    FRESULT res;
    uint8_t i;
    char *fn;


    if ((res = f_opendir(&dirs, path)) == FR_OK) {
        i = (uint8_t)strlen(path);
        while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) {
            if (_FS_RPATH && Finfo.fname[0] == '.')
                continue;
#if _USE_LFN
            fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
            fn = Finfo.fname;
#endif
            if (Finfo.fattrib & AM_DIR) {
                acc_dirs++;
                *(path + i) = '/';
                strcpy(path + i + 1, fn);
                res = scan_files(path);
                *(path + i) = '\0';
                if (res != FR_OK)
                    break;
            } else {
                //      xprintf("%s/%s\n", path, fn);
                acc_files++;
                acc_size += Finfo.fsize;
            }
        }
    }

    return res;
}

static
void put_rc(FRESULT rc)
{
    const char *str =
        "OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
        "INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
        "INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
        "LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
    FRESULT i;

    for (i = 0; i != rc && *str; i++) {
        while (*str++);
    }
    xprintf("rc=%u FR_%s\n", (uint32_t) rc, str);
}

static
void display_help(void)
{
    xputs("dd <phy_drv#> [<sector>] - Dump sector\n"
          "di <phy_drv#> - Initialize disk\n"
          "ds <phy_drv#> - Show disk status\n"
          "bd <addr> - Dump R/W buffer\n"
          "be <addr> [<data>] ... - Edit R/W buffer\n"
          "br <phy_drv#> <sector> [<n>] - Read disk into R/W buffer\n"
          "bf <n> - Fill working buffer\n"
          "fc - Close a file\n"
          "fd <len> - Read and dump file from current fp\n"
          "fe - Seek file pointer\n"
          "fi <log drv#> - Force initialize the logical drive\n"
          "fl [<path>] - Directory listing\n"
          "fo <mode> <file> - Open a file\n"
    	  "fp -  (to be added by you) \n"
          "fr <len> - Read file\n"
          "fs [<path>] - Show logical drive status\n"
          "fz [<len>] - Get/Set transfer unit for fr/fw commands\n"
          "h view help (this)\n");
}

char *fileName[30][30];
unsigned long fileSize[30];
unsigned long fileIndex[30];
int curr_index;
FILE *lcd;
int play_pause = 0;
int fileNameCount = 0;
int fileSizeCount = 0;
long p1, p2, p3, leftover;
int changed = 0;
int back_to_start = 0;
int stop = 0;
int file_size = 0;
int wavCount;

int main()
{
	//int fifospace;
    //char *ptr, *ptr2;
    long p2;
    //p1 = 0;
    uint8_t res = 0;
    uint32_t s2, cnt = sizeof(Buff);
    uint32_t ofs = 0;

    alt_up_audio_dev * audio_dev;
    /* used for audio record/playback */
    unsigned int l_buf;
    unsigned int r_buf;
    // open the Audio port
    audio_dev = alt_up_audio_open_dev ("/dev/Audio");
    if ( audio_dev == NULL)
    alt_printf ("Error: could not open audio device \n");
    else
    alt_printf ("Opened audio device \n");

	//7 = Button 3 - Previous
	//11 = Button 2 - Stop
	//13 = Button 1 - Play/Pause
	//14 = Button 0  - Next

    IoInit();

    IOWR(SEVEN_SEG_PIO_BASE,1,0x0007);

//    xputs(PSTR("FatFs module test monitor\n"));
//    xputs(_USE_LFN ? "LFN Enabled" : "LFN Disabled");
//    xprintf(", Code page: %u\n", _CODE_PAGE);

	disk_initialize((uint8_t) 0); //disk init
	//printf("disk init good");
	f_mount((uint8_t) 0, &Fatfs[0]); //mount disk
	//printf("mount good");

	//Check Buttons Interrupt
	enabled();
	//Indexes songs
	songIndex();

START:
			f_open(&File1, fileName[curr_index], (uint8_t) 1);
			file_size = fileSize[curr_index];
			display_LCD();
			ofs = File1.fptr;
			int i = 0;

			while (file_size) { //Stay in while loop as long as there song data to be played
					if(changed) { //If there was a button press
						changed = 0;
						if(back_to_start) // Previous or Next was selected
						{
							back_to_start = 0;
							goto START;
						}
						if(stop) { //Stop button was pushed
							stop = 0;
							file_size = fileSize[curr_index];
							play_pause = 0;
							//changed = 1;
							goto START;
						}

						while(!play_pause) { //play_pause initially 0 thus OFF then when pressed play_pause is 1 thus ON
							if(changed == 1) {
								if(curr_index < 0) { //Underflow for Previous
									curr_index = wavCount - 1;
									play_pause = 0;
								}
								if(curr_index > wavCount ) { //Overflow for Next
									curr_index = 0;
									play_pause = 0;
								}
								if(back_to_start) { //If Next or Previous was pressed go back to start
									back_to_start = 0;
									goto START;
								}
								if (stop == 1) {
									goto START;
								}
								display_LCD();
								changed = 0;
							}
						}
					}
					if (play_pause) {
					//Playing Audio
					if ((uint32_t) file_size >= 512)
					{
						cnt = 512;
						file_size -= 512;
					}
					else
					{
						cnt = file_size;
						file_size = 0;
						play_pause = 0;
					}
					res = f_read(&File1, Buff, cnt, &s2);
					if (res != FR_OK)
					{
						break; //Error had occured
					}
					p2 += s2; // increment p2 by the s2 referenced value
					if (cnt != s2) { //error if cnt does not equal s2 referenced value
						break;
					}

					for( i = 0 ; i < s2 ; i+=4) {
						//Left buffer
						l_buf = Buff[i+1];
						l_buf = l_buf << 8;
						l_buf = l_buf | Buff[i];

						//Right buffer
						r_buf = Buff[i+3];
						r_buf = r_buf << 8;
						r_buf = r_buf | Buff[i+2];

						while(alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_RIGHT)==0){
						}

						alt_up_audio_write_fifo (audio_dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
						alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);
					}
				}
			}
			if(curr_index < 0) { //Again checking for underflow with Previous
				curr_index = wavCount - 1;
				play_pause = 0;
				goto START;
			}

			curr_index ++;
			//play_pause = 0;

			if(curr_index > wavCount){ //Again checking for overflow with Next
				curr_index = 0;
				play_pause = 0;
			}
			goto START;
			return 0;
}

int isWav(char *filename) {
	//Determines if the file is a .wav file
	if (strstr(filename, ".WAV")!= NULL) {
		return 1;
	}
	else {
		return 0;
	}
}

void songIndex() {
	FRESULT res;
	uint32_t s1, s2, blen = sizeof(Buff);

	res = f_opendir(&Dir, 0);
	if (res) // if res in non-zero there is an error; print the error.
	{
		printf("open file error");
		put_rc(res);
	}
	for (;;)
	{
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;
//		if (Finfo.fattrib & AM_DIR)
//		{
//			s2++;
//		}
//		else
//		{
//			s1++;
//			p1 += Finfo.fsize;
//		}

		if (isWav((&Finfo.fname[0])) != 0) {
			strcpy(&(fileName[wavCount]), &(Finfo.fname[0]));
			fileSize[wavCount] = Finfo.fsize;
			fileIndex[wavCount] = fileNameCount + 1;
			wavCount++;
		}
		fileNameCount++;
		fileSizeCount++;
	}
}
void display_LCD(){

	lcd = fopen("/dev/lcd_display", "w");

	char index[10];

	sprintf(index, "%d - ", fileIndex[curr_index]);

	if(lcd != NULL) {
		fprintf(lcd, index);
		fprintf(lcd, fileName[curr_index]);
		fprintf(lcd, "\n ");
	}

	fclose(lcd);
}

//void playPause() {
//	//Do we need this function?
//	int fifospace;
//    char *ptr, *ptr2;
//    long p1, p2, p3, leftover;
//    p1 = 0;
//    uint8_t res, b1, drv = 0;
//    uint16_t w1;
//    uint32_t s1, s2, cnt, blen = sizeof(Buff);
//    static const uint8_t ft[] = { 0, 12, 16, 32 };
//    uint32_t ofs = 0, sect = 0, blk[2];
//    FATFS *fs;                  /* Pointer to file system object */
//
//    alt_up_audio_dev * audio_dev;
//    /* used for audio record/playback */
//    unsigned int l_buf;
//    unsigned int r_buf;
//    // open the Audio port
//    audio_dev = alt_up_audio_open_dev ("/dev/Audio");
//    if ( audio_dev == NULL)
//    alt_printf ("Error: could not open audio device \n");
//    else
//    alt_printf ("Opened audio device \n");
//
//		if (leftover != 0) {
//			p1 = leftover;
//		}
//		else {
//			p1 = fileSize[curr_index];
//		}
//		f_open(&File1, &(fileName[curr_index]),(uint8_t) 1);
//        while (p1)
//        {
////        	if(play_pause == 1) {
//////        		leftover = p1;
//////        		p1 = 0;
////        		continue;
////        	}
//
//            if ((uint32_t) p1 >= 256)
//            {
//                cnt = 256;
//                p1 -= 256;
//                leftover = p1;
//            }
//            else
//            {
//                cnt = p1;
//                p1 = 0;
//                //leftover = p1;
//            }
//            res = f_read(&File1, Buff, cnt, &s2);
//            if (res != FR_OK)
//            {
//                put_rc(res); // output a read error if a read error occurs
//                break;
//            }
//            if (cnt != s2) //error if cnt does not equal s2 referenced value ???
//                break;
//
//    	int i = 0;
//    	for(i = 0; i < s2; i+=4) {
//
//			l_buf = Buff[i+1];
//			l_buf = l_buf << 8;
//			l_buf = l_buf | Buff[i];
//			//printf("l_buf = %d\n", l_buf);
//
//			r_buf = Buff[i+3];
//			r_buf = r_buf << 8;
//			r_buf = r_buf | Buff[i+2];
//
//			/* read and echo audio data */
//
//			while(alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_RIGHT) == 0) {
//
//			}
//    		// write audio buffer
//    		alt_up_audio_write_fifo (audio_dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
//    		alt_up_audio_write_fifo (audio_dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);
//
//        }
//	}
////	else if (play_pause == 1){
////		//Pause current file
////		f_close(&File1);
////	}
//}
////void stop() {
////
////	leftover = 0;
////
////}
//void previous(){
//	if(curr_index == 0){
//		curr_index = fileNameCount - 1;
//		leftover = 0;
//	}
//	else{
//		curr_index--;
//		leftover = 0;
//	}
//}
//void next() {
//	if(curr_index == fileNameCount-1) {
//		curr_index = 0;
//		leftover = 0;
//	}
//	else{
//		curr_index++;
//		leftover = 0;
//	}
//}
static void check_button(void* context, alt_u32 id)
{
	volatile unsigned char button;

	//Debouncing the button
	int buttonCounter = 0;
	while(buttonCounter != 500000){
		buttonCounter ++;
	}

	button = IORD(BUTTON_PIO_BASE, 0);

	switch(button)
	{
	case 7 :
		curr_index--;
		back_to_start = 1;
		changed = 1;
		play_pause = 0;
		break;

	case 11 :
		stop = 1;
		changed = 1;
		break;

	case 13 :
		play_pause = !play_pause;
		changed = 1;
		break;

	case 14 :
		curr_index++;
		back_to_start = 1;
		changed = 1;
		play_pause = 0;
		break;
	default :
		break;
	}

	//Clear the edge of the register
	IOWR(BUTTON_PIO_BASE, 3, BUTTON_PIO_BIT_CLEARING_EDGE_REGISTER);
}

int enabled()
{
	alt_irq_register(BUTTON_PIO_IRQ, (void *)0, check_button);

	//Enable interrupt
	IOWR(BUTTON_PIO_BASE, 2, 0xF);

	return 0;
}





