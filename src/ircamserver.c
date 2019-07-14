// COMPILE:
// gcc ircamserver.c cred2struct.c -o ircamserver -I/opt/EDTpdv /opt/EDTpdv/libpdv.a -lm -lpthread -ldl


#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>           /* memory management declarations */
#include <sys/stat.h>           /*mode_t * ; data returned by the stat function */
#include <unistd.h>             /* provides acess to POSIX operating system */
#include <fcntl.h>              /* File control operations */
#include <string.h>             /* strncmp, strcpy, strcat */
#include "cred2struct.h"        /* for CREDSTRUCT */
#include "edtinc.h"             /* for EdtDev */
#include "pciload.h"            /*for strip_newline function */


#define SERBUFSIZE 512
static char buf[SERBUFSIZE+1];


//#define ircamconf_name "/tmp/ircamconf.shm"



// frame grabber index
int unit = 0;

// camera index within frame grabber
int cam = 0;



// CROP MODES
#define NBcropModes 12

int CropMode_x0[NBcropModes];
int CropMode_x1[NBcropModes];
int CropMode_y0[NBcropModes];
int CropMode_y1[NBcropModes];
float CropMode_fps[NBcropModes];
float CropMode_tint[NBcropModes];


CRED2STRUCT *ircamconf;

int verbose = 1;
int NDR_setpoint;
int cropmode_setpoint;
int x0_setpoint;
int x1_setpoint;
int y0_setpoint;
int y1_setpoint;

float fps_setpoint;
float tint_setpoint;




int init_CropModes()
{
    int i;

    // 320x256 half frame, centered 
    // cols
    CropMode_x0[0] = 160;
    CropMode_x1[0] = 479;
    // rows
    CropMode_y0[0] = 128;
    CropMode_y1[0] = 383;
    CropMode_fps[0] = 1500.082358000; //1192.300175884;
    CropMode_tint[0] = 0.000663336; //0.000828881;


    // 224 x 156, centered 
    // cols
    CropMode_x0[1] = 192;
    CropMode_x1[1] = 415;
    // rows
    CropMode_y0[1] = 160;
    CropMode_y1[1] = 347;
    CropMode_fps[1] = 2050.202611000; //1872.621240025;
    CropMode_tint[1] = 0.000483913; //0.000529585;


    // 128 x 128, centered 
    // cols
    CropMode_x0[2] = 256;
    CropMode_x1[2] = 383;
    // rows
    CropMode_y0[2] = 192;
    CropMode_y1[2] = 319;
    CropMode_fps[2] = 4500.617741000; //3192.569335426;
    CropMode_tint[2] = 0.000218568; //0.000308720;


    // 64 x 64, centered
    // cols
    CropMode_x0[3] = 288;
    CropMode_x1[3] = 351;
    // rows
    CropMode_y0[3] = 224;
    CropMode_y1[3] = 287;
    CropMode_fps[3] = 9203.638201000; //6428.872497366;
    CropMode_tint[3] = 0.000105249; //0.000151041;


    // 192 x 192, centered
    // cols
    CropMode_x0[4] = 224;
    CropMode_x1[4] = 415;
    // rows
    CropMode_y0[4] = 160;
    CropMode_y1[4] = 351;
    CropMode_fps[4] = 2200.024157000  ; //1949.824224992;
    CropMode_tint[4] = 0.000449819; //0.009995574;


	// 96 x 72, centered
	// cols
    CropMode_x0[5] = 288;
    CropMode_x1[5] = 383;
    // rows
    CropMode_y0[5] = 208;
    CropMode_y1[5] = 303;
    CropMode_fps[5] = 2000.0; //1949.824224992;
    CropMode_tint[5] = 0.0001; //0.009995574;
	

    // OFFSET -32 pix in X (Y band)
    for(i=0; i<6; i++)
    {
        // cols
        CropMode_x0[i+6] = CropMode_x0[i]-32;
        CropMode_x1[i+6] = CropMode_x1[i]-32;
        // rows
        CropMode_y0[i+6] = CropMode_y0[i];
        CropMode_y1[i+6] = CropMode_y1[i];
        CropMode_fps[i+6] = CropMode_fps[i];
        CropMode_tint[i+6] = CropMode_tint[i];
    }


    return 0;
}






int readpdvcli(EdtDev *ed, char *outbuf)
{
    int ret = 0;
    u_char  lastbyte, waitc;
    int     length=0;

    outbuf[0] = '\0';
    do
    {
        ret = pdv_serial_read(ed, buf, SERBUFSIZE);
        if (verbose)
            printf("read returned %d\n", ret);

        if (*buf)
            lastbyte = (u_char)buf[strlen(buf)-1];

        if (ret != 0)
        {
            buf[ret + 1] = 0;
            //                    fputs(buf, outbuf);
            strcat(outbuf, buf);
            length += ret;
        }

        if (ed->devid == PDVFOI_ID)
            ret = pdv_serial_wait(ed, 500, 0);
        else if (pdv_get_waitchar(ed, &waitc) && (lastbyte == waitc))
            ret = 0; /* jump out if waitchar is enabled/received */
        else ret = pdv_serial_wait(ed, 500, 64);
    } while (ret > 0);
}



int printHelp()
{
    int i;

    printf("---------------------------------------------------------------------------------\n");
    printf(" List of commands\n");
    printf("  %15s  %20s  %40s\n", "cmd",  "Parameters", "Descrition");
    printf("---------------------------------------------------------------------------------\n");
    printf("\n");
    printf("  %15s  %20s  %40s\n", "help", "", "List all commands");
    printf("  %15s  %20s  %40s\n", "readconf", "", "Read current camera settings");
    printf("  %15s  %20s  %40s\n", "exit", "", "Exit server");

    printf("  %15s  %20s  %40s\n", "raw", "", "send raw command");
    printf("  %15s  %20s  %40s\n", "reset", "", "Reset Camera");
    printf("  %15s  %20s  %40s\n", "shutdown", "", "Shutdown Camera");
    printf("\n");
    printf("  %15s  %20s  %40s\n", "gtint", "", "Get exposure time");
    printf("  %15s  %20s  %40s\n", "stint", "<etime[sec]>", "Set exposure time");
    printf("\n");
    printf("  %15s  %20s  %40s\n", "gfps","", "Get number of frames per second");
    printf("  %15s  %20s  %40s\n", "sfps", "<fps>", "Set number of frames per second");
    printf("\n");
    printf("  %15s  %20s  %40s\n", "gtemp", "", "Get detector temperature");
    printf("  %15s  %20s  %40s\n", "stemp", "<temp[C]>", "Set detector temperature");
    printf("\n");
    printf("  %15s  %20s  %40s\n", "gNDR",  "", "Get number of reads between reset");
    printf("  %15s  %20s  %40s\n", "sNDR",  "<NDR>", "Set number of reads between reset");
    printf("  %15s  %20s  %40s\n", "rawON",  "", "Set raw mode on");
    printf("  %15s  %20s  %40s\n", "rawOFF",  "", "Set raw mode off");
    printf("\n");
    printf("  %15s  %20s  %40s\n", "start", "", "Start acquistion (infinite loop)");
    printf("  %15s  %20s  %40s\n", "take", "<N>", "Take N images");
    printf("  %15s  %20s  %40s\n", "stop",  "", "Stop acquistion");
    printf("\n");
    printf("Image cropping parameters. Set parameters, and restart acquisition to take effect\n");
    printf("\n");
    printf("  %15s  %20s  %40s\n", "cropON",  "", "Set crop ON");
    printf("  %15s  %20s  %40s\n", "cropOFF", "", "Set crop OFF");
    printf("  %15s  %20s  %40s\n", "scrop_cols",  "<x0> <x1>", "Set crop window parameters");
    printf("  %15s  %20s  %40s\n", "scrop_rows",  "<y0> <y1>", "Set crop window parameters");
    printf("  %15s  %20s  %40s\n", "gcrop_cols",  "", "Get current crop window column parameters");
    printf("  %15s  %20s  %40s\n", "gcrop_rows",  "", "Get current crop window row parameters");
    printf("\n");
    printf("  %15s  %20s  %40s\n", "setcrop",  "<Mode>", "Set specific crop mode");
    printf("PREDEFINED CROP MODES: \n");
    for(i=0; i<NBcropModes; i++)
        printf("  [%d]  %3d x %3d    ( %03d-%03d x  %03d-%03d)  fps = %.1f Hz\n", i,
               (CropMode_x1[i]-CropMode_x0[i])+1,
               (CropMode_y1[i]-CropMode_y0[i])+1,
               CropMode_x0[i], CropMode_x1[i], CropMode_y0[i], CropMode_y1[i], CropMode_fps[i]);

    printf("---------------------------------------------------------------------------------\n");
    printf("\n");


    // help     List all commands
    // readconf Read current camera settings stored in SHM
    // exit     Exit server


    // gint		get integration time
    // sint		set integration time

    // gtemp	get detector temperature
    // stemp	set detector temperature

    // gfps		get frame rate
    // sfps		set frame rate

    // gNDR		get number of reads between reset
    // sNDR		set number of reads between reset


}




float servercommand_gtint(EdtDev *ed) //Get exposure time
{
    char str0[100];
    char str1[100];
    char str2[100];
    char str3[100];
    char str4[100];
    float v0;

    char    tmpbuf[SERBUFSIZE+1];
    char    serialcmd[200];
    char 	outbuf[2000];

    readpdvcli(ed, outbuf);
    sprintf(serialcmd, "tint");
    sprintf(tmpbuf, "%s\r", serialcmd);
    if (verbose)
        printf("writing <%s>\n", serialcmd);
    pdv_serial_command(ed, tmpbuf);

    readpdvcli(ed, outbuf);
    sscanf(outbuf,"%s %s %s %s %s %f", str0, str1, str2, str3, str4, &v0);
    printf("outbuf:\n%s\n",outbuf);

    return(v0);
}


float servercommand_gfps(EdtDev *ed) // Get frames per second
{
    char str0[100];
    char str1[100];
    char str2[100];
    float v0;

    char    tmpbuf[SERBUFSIZE+1];
    char    serialcmd[200];
    char 	outbuf[2000];

    readpdvcli(ed, outbuf);
    sprintf(serialcmd, "fps");
    sprintf(tmpbuf, "%s\r", serialcmd);
    if (verbose)
        printf("writing <%s>\n", serialcmd);
    pdv_serial_command(ed, tmpbuf);

    readpdvcli(ed, outbuf);
    sscanf(outbuf,"%s %s %s %f", str0, str1, str2, &v0);
    printf("outbuf:\n%s\n",outbuf);

    return(v0);
}


int servercommand_gNDR(EdtDev *ed) // Get NDR
{
    char str0[100];
    char str1[100];
    char str2[100];
    char str3[100];
    char str4[100];
    int d0;

    char    tmpbuf[SERBUFSIZE+1];
    char    serialcmd[200];
    char 	outbuf[2000];

    readpdvcli(ed, outbuf);
    sprintf(serialcmd, "nbreadworeset");
    sprintf(tmpbuf, "%s\r", serialcmd);
    if (verbose)
        printf("writing <%s>\n", serialcmd);
    pdv_serial_command(ed, tmpbuf);

    readpdvcli(ed, outbuf);
    sscanf(outbuf,"%s %s %s %s %s %d", str0, str1, str2, str3, str4, &d0);
    printf("outbuf:\n%s\n",outbuf);

    return(d0);
}


float servercommand_gtemp(EdtDev *ed) // Get Temperatures
{

    char str0[100];
    char str1[100];
    float v0;

    char tmpbuf[SERBUFSIZE+1];
    char serialcmd[200];
    char outbuf[2000];

    readpdvcli(ed, outbuf);
    sprintf(serialcmd, "temperatures snake");
    sprintf(tmpbuf, "%s\r", serialcmd);
    if (verbose)
        printf("writing <%s>\n", serialcmd);
    pdv_serial_command(ed, tmpbuf);

    readpdvcli(ed, outbuf);
    sscanf(outbuf,"%s %s %f", str0, str1, &v0);
    printf("outbuf:\n%s\n",outbuf);

    return(v0);
}








int printhelp()
{
    printf("This is the help\n");
    return 0;
}



//**************************************************************************MAIN FUNCTION **************************************************************************************//


int main(int     argc,    char  **argv)
{
    char promptstring[200];
    char prompt[200];
    char cmdstring[200];
    char command[200];
    int cam = 0;

    float v0;
    int d0;
    int d1;

    char str0[100];
    char str1[100];
    char str2[100];
    char str3[100];
    char str4[100];
    char str5[100];
    int     ret;

    EdtDev *ed;
    int unit = 0;
    int channel = 0;
    int baud = 115200;
    int timeout = 0;

    char    tmpbuf[SERBUFSIZE+1];
    char    serialcmd[200];
    char 	outbuf[2000];

    init_CropModes();

    /*
    * process command line arguments
    */
    --argc;
    ++argv;
    while (argc && ((argv[0][0] == '-') || (argv[0][0] == '/')))
    {
        switch (argv[0][1])
        {
        case 'u':
            ++argv;
            --argc;
            if (argc < 1)
            {
                printf("Error: option 'u' requires a numeric argument (0 , 1, or 2)\n");
            }
            if ((argv[0][0] >= '0') && (argv[0][0] <= '2'))
            {
                unit = atoi(argv[0]);
            }
            else
            {
                printf("Error: option 'u' requires a numeric argument (0, 1 or 2)\n");
            }
            break;

        case '-':
            if (strcmp(argv[0], "--help") == 0) {
                printhelp();
                exit(0);
            } else {
                fprintf(stderr, "unknown option: %s\n", argv[0]);
                printhelp();
                exit(1);
            }
            break;


        default:
            fprintf(stderr, "unknown flag -'%c'\n", argv[0][1]);
        case '?':
        case 'h':
            printhelp();
            exit(0);
        }
        argc--;
        argv++;
    }










    printf("IRCAM server\n");
    printf("  unit = %d\n", unit);
    printf("Type \"help\" for instructions\n");



    printf("Reading / creating shared memory structure\n");
    initCRED2STRUCT(unit);

    printCRED2STRUCT(unit);

    cam = 0;
    sprintf(promptstring, "u%dcam%d", unit, cam);
    sprintf(prompt,"%c[%d;%dm%s >%c[%dm ",0x1B, 1, 36, promptstring, 0x1B, 0);



    //*********** open a handle to the device **************************//

    ed = pdv_open_channel(EDT_INTERFACE, unit, channel);
    if (ed == NULL)
    {
        pdv_perror(EDT_INTERFACE);
        return -1;
    }

    pdv_serial_read_enable(ed);

    if (timeout < 1)
        timeout = ed->dd_p->serial_timeout;
    if (verbose)
        printf("serial timeout %d\n", timeout);

    pdv_set_baud(ed, baud);


    //************************command lines****************************//

    for (;;) {
        int cmdOK = 0;
        printf("%s ", prompt);
        fgets(cmdstring, 200, stdin);


        /****** read config file ************/

        if(cmdOK==0)
            if(strncmp(cmdstring, "readconf", strlen("readconf"))==0)
            {
                printf(" ---- unit = %d ---\n", unit);
                printCRED2STRUCT(unit);
                cmdOK = 1;
            }



        /********* help ********************/
        if(cmdOK==0)
            if(strncmp(cmdstring, "help", strlen("help"))==0)
            {
                printHelp();
                cmdOK = 1;
            }





        /********* reset ********************/
        if(cmdOK==0)
            if(strncmp(cmdstring, "reset", strlen("reset")) == 0)
            {
                sscanf(cmdstring, "%s", str0);
                sprintf(serialcmd, "restorefactory");

                printf("Reset Camera");
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("Writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);
                readpdvcli(ed, outbuf);
                printf("outbuf: \n%s\n", outbuf);

                cmdOK = 1;
            }



        /********* shutdown ********************/
        if(cmdOK==0)
            if(strncmp(cmdstring, "shutdown", strlen("shutdown")) == 0)
            {
                sscanf(cmdstring, "%s", str0);
                sprintf(serialcmd, "shutdown");

                printf("Shutdown Camera");
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("Writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);
                readpdvcli(ed, outbuf);
                printf("outbuf: \n%s\n", outbuf);

                cmdOK = 1;
            }




        /******* exposure time *************/

        if(cmdOK==0)
            if(strncmp(cmdstring, "gtint", strlen("gtint"))==0)
            {
                ircamconf[unit].tint = servercommand_gtint(ed);
                cmdOK = 1;
            }


        if(cmdOK==0)
            if(strncmp(cmdstring, "stint", strlen("stint"))==0)
            {
                sscanf(cmdstring, "%s %f", str0, &v0);
                ircamconf[unit].tint = v0;
                printf("Setting tint to %f\n", v0);


                sprintf(serialcmd, "set tint %f", v0);
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);
                readpdvcli(ed, outbuf);

                tint_setpoint = v0;
                printf("outbuf:\n%s\n",outbuf);
                ircamconf[unit].tint = servercommand_gtint(ed);
                cmdOK = 1;
            }


        /********** temperature ************/

        if(cmdOK==0)
            if(strncmp(cmdstring, "gtemp", strlen("gtemp"))==0)
            {

                ircamconf[unit].temperature = servercommand_gtemp(ed);
                cmdOK = 1;
            }

        if(cmdOK==0)
            if(strncmp(cmdstring, "stemp", strlen("stemp"))==0)
            {
                sscanf(cmdstring, "%s %f", str0, &v0);
                sprintf(serialcmd, "set temperatures snake %f", v0);
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);
                readpdvcli(ed, outbuf);


                ircamconf[unit].temperature_setpoint = v0;
                printf("outbuf:\n%s\n",outbuf);
                ircamconf[unit].temperature_setpoint = servercommand_gtemp(ed);
                cmdOK = 1;
            }



        /************** frame rate *************/

        if(cmdOK==0)
            if(strncmp(cmdstring, "gfps", strlen("gfps"))==0)
            {

                ircamconf[unit].fps = servercommand_gfps(ed);
                cmdOK = 1;
            }

        if(cmdOK==0)
            if(strncmp(cmdstring, "sfps", strlen("sfps"))==0)
            {
                sscanf(cmdstring, "%s %f", str0, &v0);
                ircamconf[unit].fps = v0;
                printf("Setting fps to %f\n", v0);
                sprintf(serialcmd, "set fps %f", v0);
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);
                readpdvcli(ed, outbuf);

                fps_setpoint = v0;
                printf("outbuf:\n%s\n",outbuf);
                ircamconf[unit].fps = servercommand_gfps(ed);
                cmdOK = 1;
            }


        /**************** NDR *******************/

        if(cmdOK==0)
            if(strncmp(cmdstring, "gNDR", strlen("gNDR"))==0)
            {
                ircamconf[unit].NDR = servercommand_gNDR(ed);
                cmdOK = 1;
            }

        if(cmdOK==0)
            if(strncmp(cmdstring, "sNDR", strlen("sNDR"))==0)
            {
                sscanf(cmdstring, "%s %d", str0, &d0);
                sprintf(serialcmd, "set nbreadworeset %d", d0);
                if(d0>255)
                    printf("Exceeding the maximum limit 255 \n");

                else {
                    ircamconf[unit].NDR = d0;
                    printf("Setting NDR to %d\n", d0);
                    sprintf(tmpbuf, "%s\r", serialcmd);
                    if (verbose)
                        printf("writing <%s>\n", serialcmd);
                    pdv_serial_command(ed, tmpbuf);
                    readpdvcli(ed, outbuf);

                    NDR_setpoint = d0;
                    printf("outbuf:\n%s\n",outbuf);
                    ircamconf[unit].NDR = servercommand_gNDR(ed);
                }
                cmdOK = 1;
            }


        if(cmdOK==0)
            if(strncmp(cmdstring, "rawON", strlen("rawON")) == 0)
            {
                sscanf(cmdstring, "%s", str0);
                sprintf(serialcmd, "set rawimages on");
                printf("Setting rawimages ON");
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("Writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);
                readpdvcli(ed, outbuf);
                printf("outbuf: \n%s\n", outbuf);

                cmdOK = 1;
            }

        if(cmdOK==0)
            if(strncmp(cmdstring, "rawOFF", strlen("rawOFF")) == 0)
            {
                sscanf(cmdstring, "%s", str0);
                sprintf(serialcmd, "set rawimages off");
                printf("Setting rawimages OFF");
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("Writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);
                readpdvcli(ed, outbuf);
                printf("outbuf: \n%s\n", outbuf);

                cmdOK = 1;
            }

        /************* imgtake process control ***************/

        if(cmdOK==0)
            if(strncmp(cmdstring, "stop", strlen("stop")) == 0)
            {
				printf("Stopping acquistion\n");
                sprintf(command, "tmux send-keys -t ircam%drun C-c", unit);
                system(command);
                cmdOK = 1;
            }

        if(cmdOK==0)
            if(strncmp(cmdstring, "take", strlen("take")) == 0)
            {				
                sscanf(cmdstring, "%s %d", str0, &d0);
                printf("Taking %d image(s)\n", d0);
                sprintf(command, "tmux send-keys -t ircam%drun \"./imgtake -u %d -l %d\" C-m", unit, unit, d0);
                system(command);
                cmdOK = 1;
            }

        if(cmdOK==0)
            if(strncmp(cmdstring, "start", strlen("start")) == 0)
            {
				printf("Starting acquisition\n");
                sprintf(command, "tmux send-keys -t ircam%drun \"./imgtake -u %d -l 0\" C-m", unit, unit);
                system(command);
                sleep(1.0);
                system("./imgtakeCPUconf");
                cmdOK = 1;
            }



        /************* cropping ***************/

        if(cmdOK==0)
            if(strncmp(cmdstring, "setcrop", strlen("setcrop")) == 0)
            {
                sscanf(cmdstring, "%s %d", str0, &d0);
                if(d0<NBcropModes)
                {
                    printf("Setting crop mode %d\n", d0);




                    printf("--------------------------------------------\n");
                    sprintf(command, "tmux send-keys -t ircam%drun C-c", unit);
                    system(command);

                    sprintf(serialcmd, "set cropping on");
                    ircamconf[unit].cropmode = 1;
                    printf("Setting crop mode ON");
                    sprintf(tmpbuf, "%s\r", serialcmd);
                    if (verbose)
                        printf("  COMMAND : %s\n", serialcmd);
                    pdv_serial_command(ed, tmpbuf);
                    readpdvcli(ed, outbuf);
                    printf("outbuf: \n%s\n", outbuf);


                    sprintf(serialcmd, "set cropping columns %d-%d", CropMode_x0[d0], CropMode_x1[d0]);
                    sprintf(tmpbuf, "%s\r", serialcmd);
                    printf("  COMMAND : %s\n", serialcmd);
                    pdv_serial_command(ed, tmpbuf);
                    readpdvcli(ed, outbuf);
                    printf("outbuf: \n%s\n", outbuf);
                    ircamconf[unit].x0 = CropMode_x0[d0];
                    ircamconf[unit].x1 = CropMode_x1[d0];
                    sleep(2);




                    sprintf(serialcmd, "set cropping columns %d-%d", CropMode_x0[d0], CropMode_x1[d0]);
                    sprintf(tmpbuf, "%s\r", serialcmd);
                    printf("  COMMAND : %s\n", serialcmd);
                    pdv_serial_command(ed, tmpbuf);
                    readpdvcli(ed, outbuf);
                    printf("outbuf: \n%s\n", outbuf);
                    ircamconf[unit].x0 = CropMode_x0[d0];
                    ircamconf[unit].x1 = CropMode_x1[d0];
                    sleep(2);


                    sprintf(serialcmd, "set cropping rows %d-%d", CropMode_y0[d0], CropMode_y1[d0]);
                    sprintf(tmpbuf, "%s\r", serialcmd);
                    printf("  COMMAND : %s\n", serialcmd);
                    pdv_serial_command(ed, tmpbuf);
                    readpdvcli(ed, outbuf);
                    printf("outbuf: \n%s\n", outbuf);
                    ircamconf[unit].y0 = CropMode_y0[d0];
                    ircamconf[unit].y1 = CropMode_y1[d0];
                    sleep(2);


                    sprintf(serialcmd, "set fps %.1f", CropMode_fps[d0]);
                    sprintf(tmpbuf, "%s\r", serialcmd);
                    printf("  COMMAND : %s\n", serialcmd);
                    pdv_serial_command(ed, tmpbuf);
                    readpdvcli(ed, outbuf);
                    printf("outbuf: \n%s\n", outbuf);
                    sleep(1);

                    sprintf(serialcmd, "set tint %.10f", CropMode_tint[d0]);
                    sprintf(tmpbuf, "%s\r", serialcmd);
                    printf("  COMMAND : %s\n", serialcmd);
                    pdv_serial_command(ed, tmpbuf);
                    readpdvcli(ed, outbuf);
                    printf("outbuf: \n%s\n", outbuf);
                    sleep(1);

                    sprintf(command, "tmux send-keys -t ircam%drun \"./imgtake -u %d -l 0\" C-m", unit, unit);
                    system(command);
                }
                cmdOK = 1;
            }


        if(cmdOK==0)
            if(strncmp(cmdstring, "cropON", strlen("cropON")) == 0)
            {
                sscanf(cmdstring, "%s", str0);
                sprintf(serialcmd, "set cropping on");
                ircamconf[unit].cropmode = 1;
                printf("Setting crop mode ON");
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("Writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);
                readpdvcli(ed, outbuf);
                printf("outbuf: \n%s\n", outbuf);

                cmdOK = 1;
            }



        if(cmdOK==0)
            if(strncmp(cmdstring, "cropOFF", strlen("cropOFF"))==0)
            {
                sscanf(cmdstring, "%s", str0);
                sprintf(serialcmd, "set cropping off");

                ircamconf[unit].cropmode = 0;

                ircamconf[unit].x0 = 0;
                ircamconf[unit].x1 = 639;

                ircamconf[unit].y0 = 0;
                ircamconf[unit].y1 = 511;

                printf("--------------------------------------------\n");
                sprintf(command, "tmux send-keys -t ircam%drun C-c", unit);
                system(command);

                printf("Setting crop mode OFF");
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("Writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);
                readpdvcli(ed, outbuf);
                printf("outbuf: \n%s\n", outbuf);

                sleep(2);

                sprintf(command, "tmux send-keys -t ircam%drun \"./imgtake -u %d -l 0\" C-m", unit, unit);
                system(command);

                cmdOK = 1;
            }



        if(cmdOK == 0)
            if(strncmp(cmdstring, "gcrop_cols", strlen("gcrop_cols"))==0)
            {
                readpdvcli(ed, outbuf);
                sprintf(serialcmd, "cropping columns");
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);

                readpdvcli(ed, outbuf);
                sscanf(outbuf,"%s %d %d", str0, &d0, &d1);
                ircamconf[unit].x0 = d0;
                ircamconf[unit].x1 = -d1;
                printf("outbuf:\n%s\n",outbuf);

                cmdOK = 1;
            }




        if(cmdOK == 0)
            if(strncmp(cmdstring, "scrop_cols", strlen("scrop_cols"))==0)
            {
                sscanf(cmdstring, "%s %d %d", str0, &d0, &d1 );
                if((d1-d0 +1)%32 == 0)
                {
                    sprintf(serialcmd, "set cropping columns %d-%d", d0, d1);
                    ircamconf[unit].x0 = d0;
                    ircamconf[unit].x1 = d1;
                    printf("cropping columns(granularity 32 \n)");
                    sprintf(tmpbuf, "%s\r", serialcmd);
                    if (verbose)
                        printf("Writing <%s>\n", serialcmd);
                    pdv_serial_command(ed, tmpbuf);
                    readpdvcli(ed, outbuf);
                    printf("outbuf: \n%s\n", outbuf);
                }
                else
                    printf("Error! Enter column values of granularity 32.\n");

                cmdOK = 1;
            }



        if(cmdOK == 0)
            if(strncmp(cmdstring, "gcrop_rows", strlen("gcrop_rows"))==0)
            {
                readpdvcli(ed, outbuf);
                sprintf(serialcmd, "cropping rows");
                sprintf(tmpbuf, "%s\r", serialcmd);
                if (verbose)
                    printf("writing <%s>\n", serialcmd);
                pdv_serial_command(ed, tmpbuf);

                readpdvcli(ed, outbuf);
                sscanf(outbuf,"%s %d %d", str0, &d0, &d1);
                ircamconf[unit].y0 = d0;
                ircamconf[unit].y1 = -d1;
                printf("outbuf:\n%s\n",outbuf);

                cmdOK = 1;
            }




        if(cmdOK == 0)
            if(strncmp(cmdstring, "scrop_rows", strlen("scrop_rows"))==0)
            {
                sscanf(cmdstring, "%s %d %d", str0, &d0, &d1 );
                if((d1-d0+1)%4 == 0)
                {
                    sprintf(serialcmd, "set cropping rows %d-%d", d0, d1);
                    ircamconf[unit].y0 = d0;
                    ircamconf[unit].y1 = d1;
                    printf("cropping columns (granularity 4 \n)");
                    sprintf(tmpbuf, "%s\r", serialcmd);
                    if (verbose)
                        printf("Writing <%s>\n", serialcmd);
                    pdv_serial_command(ed, tmpbuf);
                    readpdvcli(ed, outbuf);
                    printf("outbuf: \n%s\n", outbuf);
                }
                else
                    printf("Error!Enter row values of granularity 4.\n");

                cmdOK = 1;
            }


        /*************** exit ****************/

        if(cmdOK==0)
            if(strncmp(cmdstring, "exit", strlen("exit"))==0)
            {
                printf("bye!\n");
                exit(0);
            }


        /************* Unknown command ******/
        if(cmdOK==0)
        {
            printf("Unknown command:  %s  -> send as raw\n", cmdstring);
            sprintf(tmpbuf, "%s\r", cmdstring);
            if (verbose)
                printf("Writing <%s>\n", cmdstring);
            pdv_serial_command(ed, tmpbuf);
            readpdvcli(ed, outbuf);
            printf("outbuf: \n%s\n", outbuf);

        }

    }


    return(0);
}


//*****************************************************************************END**********************************************************************************//
