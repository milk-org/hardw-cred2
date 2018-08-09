#ifndef _CRED2STRUCT_H
#define _CRED2STRUCT_H
#define ircamconf_name "/tmp/ircamconf.shm"

#define NBconf  2


typedef struct
{		
	int FGchannel;          // frame grabber channel
	
	float tint;             // integration time for each read	
	int   NDR;              // number of reads per reset	
    float fps;              // current number of frames in Hz

	float temperature;
	float temperature_setpoint;
	
		
	// cropping parameters
	int cropmode;          // 0: OFF, 1: ON
	int x0;
	int x1;
	int y0;
	int y1;
	
		
	int sensibility;
	// 0: low
	// 1: medium
	// 2: high
	
	long frameindex;
	
} CRED2STRUCT;


int printCRED2STRUCT(int cam);
int initCRED2STRUCT();


#endif
