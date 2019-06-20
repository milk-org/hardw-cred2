#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cred2struct.h"



extern CRED2STRUCT *ircamconf;

int printCRED2STRUCT(int unit)
{
	printf("unit = %d\n", unit);
	printf("======================================================================\n");
	printf("FGchannel    %d\n", ircamconf[unit].FGchannel);
	printf("tint         %f\n", ircamconf[unit].tint);
	printf("NDR val      %d\n", ircamconf[unit].NDR);


	printf("Frame rate             %12f Hz\n", ircamconf[unit].fps);	
	
	printf("Detector temperature   %12f  (setpt = %12f)\n", 
	ircamconf[unit].temperature,
	ircamconf[unit].temperature_setpoint);

	printf("cropping   ON=%d   window:\n",ircamconf[unit].cropmode);
	printf("    x / cols   :  %3d-%3d   size = %3d\n", ircamconf[unit].x0, ircamconf[unit].x1, (ircamconf[unit].x1-ircamconf[unit].x0)+1);
	printf("    y / rows   :  %3d-%3d   size = %3d\n", ircamconf[unit].y0, ircamconf[unit].y1, (ircamconf[unit].y1-ircamconf[unit].y0)+1);
		
	printf("Current frame = %ld\n", ircamconf[unit].frameindex);
	printf("======================================================================\n");

	return (0);
}




int initCRED2STRUCT(int unit)
{
	int SM_fd;        // shared memory file descriptor
	int create = 0;   // 1 if we need to re-create shared memory
	struct stat file_stat;
	char confname[100];
	
	sprintf(confname, "/tmp/ircamconf%d.shm", unit);
	
	SM_fd = open(confname, O_RDWR);
    if(SM_fd==-1)
    {
        printf("Cannot import file \"%s\" -> creating file\n", confname);
        create = 1;
    }
    else
    {
        fstat(SM_fd, &file_stat);  // read file stats
        printf("File %s size: %zd\n", confname, file_stat.st_size);
        if(file_stat.st_size!=sizeof(CRED2STRUCT)*NBconf)
        {
            printf("File \"%s\" size is wrong -> recreating file\n", confname);
            create = 1;
            close(SM_fd);
        }
    }

    if(create==1)
    {
		printf("======== CREATING SHARED MEMORY FILE =======\n");
        int result;

        SM_fd = open(confname, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);

        if (SM_fd == -1) {
            perror("Error opening file for writing");
            exit(0);
        }

        result = lseek(SM_fd, sizeof(CRED2STRUCT)*NBconf-1, SEEK_SET);
        if (result == -1) {
            close(SM_fd);
            perror("Error calling lseek() to 'stretch' the file");
            exit(0);
        }

        result = write(SM_fd, "", 1);
        if (result != 1) {
            close(SM_fd);
            perror("Error writing last byte of the file");
            exit(0);
        }
    }


    ircamconf = (CRED2STRUCT*) mmap(0, sizeof(CRED2STRUCT)*NBconf, PROT_READ | PROT_WRITE, MAP_SHARED, SM_fd, 0);
    if (ircamconf == MAP_FAILED) {
        close(SM_fd);
        perror("Error mmapping the file");
        exit(0);
    }
	
	return 0;
}




