#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cred2struct.h"



extern CRED2STRUCT *ircamconf;

int printCRED2STRUCT(int cam)
{
	printf("cam = %d\n", cam);
	printf("======================================================================\n");
	printf("FGchannel    %d\n", ircamconf[cam].FGchannel);
	printf("tint         %f\n", ircamconf[cam].tint);
	printf("NDR val      %d\n", ircamconf[cam].NDR);


	printf("Frame rate             %12f Hz\n", ircamconf[cam].fps);	
	
	printf("Detector temperature   %12f  (setpt = %12f)\n", 
	ircamconf[cam].temperature,
	ircamconf[cam].temperature_setpoint);

	printf("cropping   ON=%d   window:\n",ircamconf[cam].cropmode);
	printf("    x / cols   :  %3d-%3d   size = %3d\n", ircamconf[cam].x0, ircamconf[cam].x1, (ircamconf[cam].x1-ircamconf[cam].x0)+1);
	printf("    y / rows   :  %3d-%3d   size = %3d\n", ircamconf[cam].y0, ircamconf[cam].y1, (ircamconf[cam].y1-ircamconf[cam].y0)+1);
		
	printf("Current frame = %ld\n", ircamconf[cam].frameindex);
	printf("======================================================================\n");

	return (0);
}




int initCRED2STRUCT()
{
	int SM_fd;        // shared memory file descriptor
	int create = 0;   // 1 if we need to re-create shared memory
	struct stat file_stat;
	
	SM_fd = open(ircamconf_name, O_RDWR);
    if(SM_fd==-1)
    {
        printf("Cannot import file \"%s\" -> creating file\n", ircamconf_name);
        create = 1;
    }
    else
    {
        fstat(SM_fd, &file_stat);  // read file stats
        printf("File %s size: %zd\n", ircamconf_name, file_stat.st_size);
        if(file_stat.st_size!=sizeof(CRED2STRUCT)*NBconf)
        {
            printf("File \"%s\" size is wrong -> recreating file\n", ircamconf_name);
            create = 1;
            close(SM_fd);
        }
    }

    if(create==1)
    {
		printf("======== CREATING SHARED MEMORY FILE =======\n");
        int result;

        SM_fd = open(ircamconf_name, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);

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




