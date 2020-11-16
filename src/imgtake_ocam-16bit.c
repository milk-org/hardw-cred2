/*
 *
 *
 * Compile with:
 * gcc imgtake.c -o imgtake -I/opt/EDTpdv
 * -I/home/scexao/src/cacao/src/ImageStreamIO
 * -I/home/scexao/src/cacao/src
 * /home/scexao/src/cacao/src/ImageStreamIO/ImageStreamIO.c
 *
 * /opt/EDTpdv/libpdv.a -lm -lpthread -ldl
 *
 *
 */


#define _GNU_SOURCE


#include <sched.h>
#include <unistd.h>



#include "edtinc.h"
#include "ImageStruct.h"
#include "ImageStreamIO.h"


static void usage(char *progname, char *errmsg);




int main(int argc, char **argv)
{
    int     i;
    int     unit = 0;
    int     overrun, overruns = 0;
    int     timeout;
    int     timeouts, last_timeouts = 0;
    int     recovering_timeout = FALSE;
    char   *progname ;
    char   *cameratype;
    int     numbufs = 4;
    u_char *image_p;
    PdvDev *pdv_p;
    char    errstr[64];
    int     loops = 1;
    int     width, height, depth;
    char    edt_devname[128];
    int     channel = 0; // same as cam
    char    streamname[200];


    unsigned short *imageshort;
    float exposure = 0.05; // exposure time [ms]

    int xsize, ysize;
    int kw;


    uid_t ruid; // Real UID (= user launching process at startup)
    uid_t euid; // Effective UID (= owner of executable at startup)
    uid_t suid; // Saved UID (= owner of executable at startup)


    int RT_priority = 70; //any number from 0-99
    struct sched_param schedpar;
    int ret;



    getresuid(&ruid, &euid, &suid);
    //This sets it to the privileges of the normal user
    ret = seteuid(ruid);
    if(ret != 0)
    {
        printf("setuid error\n");
    }




    schedpar.sched_priority = RT_priority;
#ifndef __MACH__
    if(ret != 0)
    {
        printf("setuid error\n");
    }
    ret = seteuid(euid); //This goes up to maximum privileges
    sched_setscheduler(0, SCHED_FIFO,
                       &schedpar); //other option is SCHED_RR, might be faster
    ret = seteuid(ruid);//Go back to normal privileges
    if(ret != 0)
    {
        printf("setuid error\n");
    }
#endif

    int STREAMNAMEINIT = 0;

    progname = argv[0];

    edt_devname[0] = '\0';

    /*
     * process command line arguments
     */
    --argc;
    ++argv;
    while(argc && ((argv[0][0] == '-') || (argv[0][0] == '/')))
    {
        switch(argv[0][1])
        {
            case 'N':
                ++argv;
                --argc;
                if(argc < 1)
                {
                    usage(progname, "Error: option 'N' requires a numeric argument\n");
                }
                if((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    numbufs = atoi(argv[0]);
                }
                else
                {
                    usage(progname, "Error: option 'N' requires a numeric argument\n");
                }
                break;


            case 's':
                ++argv;
                --argc;
                if(argc < 1)
                {
                    printf("Error: option 's' requires string argument\n");
                }
                strcpy(streamname, argv[0]);
                STREAMNAMEINIT = 1;
                break;
                

            case 'u':
                ++argv;
                --argc;
                if(argc < 1)
                {
                    printf("Error: option 'u' requires a numeric argument (0, 1 or 2)\n");
                }
                if((argv[0][0] >= '0') && (argv[0][0] <= '2'))
                {
                    unit = atoi(argv[0]);
                }
                else
                {
                    printf("Error: option 'u' requires a numeric argument (0, 1 or 2)\n");
                }
                break;

            case 'c':
                ++argv;
                --argc;
                if(argc < 1)
                {
                    printf("Error: option 'c' requires a numeric argument (0 or 1)\n");
                }
                if((argv[0][0] >= '0') && (argv[0][0] <= '2'))
                {
                    channel = atoi(argv[0]);
                }
                else
                {
                    printf("Error: option 'c' requires a numeric argument (0 or 1)\n");
                }
                break;

            case 'l':
                ++argv;
                --argc;
                if(argc < 1)
                {
                    usage(progname, "Error: option 'l' requires a numeric argument\n");
                }
                if((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    loops = atoi(argv[0]);
                }
                else
                {
                    usage(progname, "Error: option 'l' requires a numeric argument\n");
                }
                break;

            case '-':
                if(strcmp(argv[0], "--help") == 0)
                {
                    usage(progname, "");
                    exit(0);
                }
                else
                {
                    fprintf(stderr, "unknown option: %s\n", argv[0]);
                    usage(progname, "");
                    exit(1);
                }
                break;


            default:
                fprintf(stderr, "unknown flag -'%c'\n", argv[0][1]);
            case '?':
            case 'h':
                usage(progname, "");
                exit(0);
        }
        argc--;
        argv++;
    }

    /*
     * open the interface
     *
     * EDT_INTERFACE is defined in edtdef.h (included via edtinc.h)
     *
     * edt_parse_unit_channel and pdv_open_channel) are equivalent to
     * edt_parse_unit and pdv_open except for the extra channel arg and
     * would normally be 0 unless there's another camera (or simulator)
     * on the second channel (camera link) or daisy-chained RCI (PCI FOI)
     */


    if(edt_devname[0])
    {
        unit = edt_parse_unit_channel(edt_devname, edt_devname, EDT_INTERFACE,
                                      &channel);
    }
    else
    {
        strcpy(edt_devname, EDT_INTERFACE);
    }

    printf("edt_devname = %s   unit = %d    channel = %d\n", edt_devname, unit,
           channel);
    int cam = 0;

    if((pdv_p = pdv_open_channel(edt_devname, unit, channel)) == NULL)
    {
        sprintf(errstr, "pdv_open_channel(%s%d_%d)", edt_devname, unit, channel);
        pdv_perror(errstr);
        return (1);
    }

    pdv_flush_fifo(pdv_p);



    IMAGE *imarray;    // pointer to array of images
    int NBIMAGES = 5;  // can hold multiple images
    long naxis;        // number of axis
    uint8_t atype;     // data type
    uint32_t *imsize;  // image size
    int shared;        // 1 if image in shared memory
    int NBkw;          // number of keywords supported

    width = pdv_get_width(pdv_p);
    height = pdv_get_height(pdv_p);
    depth = pdv_get_depth(pdv_p);
    timeout = pdv_get_timeout(pdv_p);
    cameratype = pdv_get_cameratype(pdv_p);

    printf("image size  : %d x %d\n", width, height);
    printf("Timeout     : %d\n", timeout);
    printf("Camera type : %s\n", cameratype);




    // allocate memory for array of images
    imarray = (IMAGE *) malloc(sizeof(IMAGE) * NBIMAGES);
    naxis = 2;
    imsize = (uint32_t *) malloc(sizeof(uint32_t) * naxis);
    imsize[0] = width / 2; // DIVISION BY 2 ACCOUNTING FOR char -> short cast
    imsize[1] = height;
    atype = _DATATYPE_UINT16;
    // image will be in shared memory
    shared = 1;
    // allocate space for 10 keywords
    NBkw = 10;

	if(STREAMNAMEINIT == 0)
	{
		sprintf(streamname, "ircam%d", unit);
		STREAMNAMEINIT = 1;
	}
	
    ImageStreamIO_createIm(&imarray[0], streamname, naxis, imsize, atype, shared,
                           NBkw);
    free(imsize);



    // SAVING CUBES TO DISK
    // CHANGE NBIMAGES to 3
    int SAVECUBE =
        0; // change to 1 when saving -> move to shared mem for interactive control
    int CUBEindex = 0; // 0 or 1
    long frameindex = 0;
    char imnamec0[200];
    char imnamec1[200];
    uint32_t CUBEsize = 1000;
    naxis = 3;
    imsize = (uint32_t *) malloc(sizeof(uint32_t) * naxis);
    imsize[0] = width;
    imsize[1] = height;
    imsize[2] = CUBEsize;
    atype = _DATATYPE_UINT16;    
    sprintf(imnamec0, "%s_cube0", streamname);
    ImageStreamIO_createIm(&imarray[1], imnamec0, naxis, imsize, atype, shared,
                           NBkw);
    sprintf(imnamec1, "%s_cube1", streamname);
    ImageStreamIO_createIm(&imarray[2], imnamec1, naxis, imsize, atype, shared,
                           NBkw);
                           
    free(imsize);




    // Add keywords
    kw = 0;
    strcpy(imarray[0].kw[kw].name, "tint");
    imarray[0].kw[kw].type = 'D';
    strcpy(imarray[0].kw[kw].comment, "exposure time");

    kw = 1;
    strcpy(imarray[0].kw[kw].name, "fps");
    imarray[0].kw[kw].type = 'D';
    strcpy(imarray[0].kw[kw].comment, "frame rate");

    kw = 2;
    strcpy(imarray[0].kw[kw].name, "NDR");
    imarray[0].kw[kw].type = 'L';
    strcpy(imarray[0].kw[kw].comment, "NDR");

    kw = 3;
    strcpy(imarray[0].kw[kw].name, "x0");
    imarray[0].kw[kw].type = 'L';
    strcpy(imarray[0].kw[kw].comment, "x0");

    kw = 4;
    strcpy(imarray[0].kw[kw].name, "x1");
    imarray[0].kw[kw].type = 'L';
    strcpy(imarray[0].kw[kw].comment, "x1");

    kw = 5;
    strcpy(imarray[0].kw[kw].name, "y0");
    imarray[0].kw[kw].type = 'L';
    strcpy(imarray[0].kw[kw].comment, "y0");

    kw = 6;
    strcpy(imarray[0].kw[kw].name, "y1");
    imarray[0].kw[kw].type = 'L';
    strcpy(imarray[0].kw[kw].comment, "y1");

    kw = 7;
    strcpy(imarray[0].kw[kw].name, "temp");
    imarray[0].kw[kw].type = 'D';
    strcpy(imarray[0].kw[kw].comment, "detector temperature");


    fflush(stdout);

    pdv_set_exposure(pdv_p, exposure);



    /*
     * allocate four buffers for optimal pdv ring buffer pipeline (reduce if
     * memory is at a premium)
     */
    pdv_multibuf(pdv_p, numbufs);

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
           loops, loops == 1 ? "" : "s", cameratype, width, height, depth);
    printf("exposure = %f\n", exposure);


    /*
     * prestart the first image or images outside the loop to get the
     * pipeline going. Start multiple images unless force_single set in
     * config file, since some cameras (e.g. ones that need a gap between
     * images or that take a serial command to start every image) don't
     * tolerate queueing of multiple images
     */
    if(pdv_p->dd_p->force_single)
    {
        pdv_start_image(pdv_p);
    }
    else
    {
        pdv_start_images(pdv_p, numbufs);
    }
    printf("\n");
    i = 0;
    int loopOK = 1;
    while(loopOK == 1)
    {
        /*
         * get the image and immediately start the next one (if not the last
         * time through the loop). Processing (saving to a file in this case)
         * can then occur in parallel with the next acquisition
         */

        image_p = pdv_wait_image(pdv_p);

        if((overrun = (edt_reg_read(pdv_p, PDV_STAT) & PDV_OVERRUN)))
        {
            ++overruns;
        }

        pdv_start_image(pdv_p);
        timeouts = pdv_timeouts(pdv_p);

        /*
         * check for timeouts or data overruns -- timeouts occur when data
         * is lost, camera isn't hooked up, etc, and application programs
         * should always check for them. data overruns usually occur as a
         * result of a timeout but should be checked for separately since
         * ROI can sometimes mask timeouts
         */
        if(timeouts > last_timeouts)
        {
            /*
             * pdv_timeout_cleanup helps recover gracefully after a timeout,
             * particularly if multiple buffers were prestarted
             */
            pdv_timeout_restart(pdv_p, TRUE);
            last_timeouts = timeouts;
            recovering_timeout = TRUE;
            printf("\ntimeout....\n");
        }
        else if(recovering_timeout)
        {
            pdv_timeout_restart(pdv_p, TRUE);
            recovering_timeout = FALSE;
            printf("\nrestarted....\n");
        }

        // printf("line = %d\n", __LINE__);
        fflush(stdout);


        imageshort = (unsigned short *) image_p;


        fflush(stdout);

        imarray[0].md[0].write = 1; // set this flag to 1 when writing data
        
        memcpy(imarray[0].array.UI16, imageshort,
               sizeof(unsigned char)*width * height);


        // SAVING CUBES TO DISK
        if(SAVECUBE == 1)
        {
            char *destptr;
            destptr = (char *) imarray[CUBEindex + 1].array.UI8 + sizeof(
                          unsigned char) * width * height * frameindex;

            memcpy((void *) destptr, imageshort, sizeof(unsigned char)*width * height);
            frameindex++;

            if(frameindex == CUBEsize)
            {
                imarray[CUBEindex + 1].md[0].cnt0 ++;
                imarray[CUBEindex + 1].md[0].cnt1 ++;
                imarray[CUBEindex + 1].md[0].write = 0;
                frameindex = 0;
                CUBEindex++;
                if(CUBEindex == 2)
                {
                    CUBEindex = 0;
                }

                imarray[CUBEindex + 1].md[0].write = 1;
            }

        }

        fflush(stdout);

        imarray[0].md[0].write = 0;
        // POST ALL SEMAPHORES
        ImageStreamIO_sempost(&imarray[0], -1);

        imarray[0].md[0].write = 0; // Done writing data
        imarray[0].md[0].cnt0++;
        imarray[0].md[0].cnt1++;

        i++;
        if(i == loops)
        {
            loopOK = 0;
        }

    }
    puts("");

    printf("%d images %d timeouts %d overruns\n", loops, last_timeouts, overruns);

    /*
     * if we got timeouts it indicates there is a problem
     */
    if(last_timeouts)
    {
        printf("check camera and connections\n");
    }
    pdv_close(pdv_p);

    if(overruns || timeouts)
    {
        exit(2);
    }


    free(imarray);

    exit(0);
}






static void
usage(char *progname, char *errmsg)
{
    puts(errmsg);
    printf("%s: simple example program that acquires images from an\n", progname);
    printf("EDT digital imaging interface board (PCI DV, PCI DVK, etc.)\n");
    puts("");
    printf("usage: %s [-n streamname] [-l loops] [-N numbufs] [-u unit] [-c channel]\n",
           progname);
    printf("  -s streamname   output stream name (default: ircam<unit>\n");
    printf("  -u unit         set unit\n");
    printf("  -c chan         set channel (1 tap, 1 cable)\n");
    printf("  -l loops        number of loops (images to take)\n");
    printf("  -N numbufs      number of ring buffers (see users guide) (default 4)\n");
    printf("  -h              this help message\n");
    exit(1);
}
