/* 
 * This code released into the public domain 21 July 2008
 * 
 * This program does the equivalent of:
 * gphoto2 --shell
 *   > set-config capture=1
 *   > capture-image-and-download
 * compile with gcc -Wall -o canon-capture -lgphoto2 canon-capture.c
 *
 * Taken from: http://credentiality2.blogspot.com/2008/07/linux-libgphoto2-image-capture-from.html 
 *
 * and condensed into simple capture sample
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdbool.h>
#include <gphoto2/gphoto2.h>

#include "samples.h"

#define _ESC_                       (27)
#define MAX_ROTAR_STEPS             (24)        // Azimuth: 360/15 degrees = 24 steps.
#define MAX_HEMISPHERES             (2)         // Product one side and fliped side will


#define BARCODE_DISABLED

static void errordumper(GPLogLevel level, const char *domain, const char *str, void *data) {
    fprintf(stdout, "%s\n", str);
}


#if defined(BARCODE_ENABLED)
/*
 * Bardcode reader.
 */ 

static int bcr_fd;

static void init_barcode_com(char* bcr_portname)
{
	/* Open the file descriptor in non-blocking mode */
	bcr_fd = open(bcr_portname, O_RDONLY | O_NOCTTY | O_NDELAY);
	if (bcr_fd == -1)
	{
        printf("ERROR: %d Cannot open fd for barcode communication with error %s \n", 
                errno, strerror(errno));
        exit (-1);
	}
    printf("Barcode Reader FD: %d\n", bcr_fd);

    fcntl(bcr_fd, F_SETFL, 0);

	/* Set up the control structure */
	struct termios toptions;
 
 	/* Get currently set options for the tty */
 	tcgetattr(bcr_fd, &toptions);
 
	/* Set custom options */
 
	/* baud */
 	cfsetispeed(&toptions, B9600);
 	cfsetospeed(&toptions, B9600);
	/* 8 bits, no parity, no stop bits */
	toptions.c_cflag &= ~PARENB;
 	toptions.c_cflag &= ~CSTOPB;
 	toptions.c_cflag &= ~CSIZE;
 	toptions.c_cflag |= CS8;
 	/* no hardware flow control */
 	toptions.c_cflag &= ~CRTSCTS;
 	/* enable receiver, ignore status lines */
 	toptions.c_cflag |= CREAD | CLOCAL;
 	/* disable input/output flow control, disable restart chars */
 	toptions.c_iflag &= ~(IXON | IXOFF | IXANY);
 	/* disable canonical input, disable echo,
 	 * disable visually erase chars,
 	 * disable terminal-generated signals */
 	toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
 	/* disable output processing */
 	toptions.c_oflag &= ~OPOST;
 
	/* wait for n (in our case its 1) characters to come in before read returns */
	/* WARNING! THIS CAUSES THE read() TO BLOCK UNTIL ALL */
	/* CHARACTERS HAVE COME IN! */
 	toptions.c_cc[VMIN] = 0;
 	/* no minimum time to wait before read returns */
 	toptions.c_cc[VTIME] = 100;

 	/* commit the options */
 	tcsetattr(bcr_fd, TCSANOW, &toptions);
 
	/* Wait for the Barcode to reset */
 	usleep(10*1000);

    return;
}

static int read_from_barcode_reader(char* bcr_buf)
{
    int i = 0, nbytes = 0;
    char buf[1];

 	/* Flush anything already in the serial buffer */
 	tcflush(bcr_fd, TCIFLUSH);

    while (1) {
        nbytes = read(bcr_fd, buf, 1);           // read a char at a time
        if (nbytes == -1) {
            return -1;                       // Couldn't read
        }
        if (nbytes == 0) {
            return 0;
        }
        if (buf[0] == '\n' || buf[0] == '\r') {
            return 0;
        }
        bcr_buf[i] = buf[0];
        i++;
    }
    bcr_buf[nbytes] = 0;
    
    return 0;
}

static void close_bcr(void)
{
    close(bcr_fd);
}
#endif


static void
capture_to_memory(Camera *camera, GPContext *context, const char **ptr, unsigned long int *size) {
	int retval;
	CameraFile *file;
	CameraFilePath camera_file_path;

	printf("Capturing.\n");

	/* NOP: This gets overridden in the library to /capt0000.jpg */
	strcpy(camera_file_path.folder, "/");
	strcpy(camera_file_path.name, "foo.jpg");

	retval = gp_camera_capture(camera, GP_CAPTURE_IMAGE, &camera_file_path, context);
	printf("  Retval: %d\n", retval);

	printf("Pathname on the camera: %s/%s\n", camera_file_path.folder, camera_file_path.name);

	retval = gp_file_new(&file);
	printf("  Retval: %d\n", retval);
	retval = gp_camera_file_get(camera, camera_file_path.folder, camera_file_path.name,
		     GP_FILE_TYPE_NORMAL, file, context);
	printf("  Retval: %d\n", retval);

	gp_file_get_data_and_size (file, ptr, size);

	printf("Deleting.\n");
	retval = gp_camera_file_delete(camera, camera_file_path.folder, camera_file_path.name,
			context);
	printf("  Retval: %d\n", retval);
	/*gp_file_free(file); */
}


/* Check if the directory exists, if not create it
 * This function will create a new directory if the image is the first
 * image taken for a specific day
 */
static inline bool directory_exists_or_create(const char* pzPath)
{
    DIR *pDir;

    /* directory doesn't exists -> create it */
    if ( pzPath == NULL || (pDir = opendir(pzPath)) == NULL) 
    {
        mkdir(pzPath, 0777);
    }
                        
    /* if directory exists we opened it just above and now we
     * have to close the directory back again.
     */
    else if(pDir != NULL) 
    {
        (void) closedir(pDir);
        return TRUE;
    }

    return FALSE;
}


static void press_enter(void)
{
    char enter = 0;
    while (enter != '\r' && enter != '\n')
    {
        enter = getchar();
    }
}


int main(int argc, char **argv) 
{
	Camera	*camera;
	int	retval;
	GPContext *context = sample_create_context();
	FILE 	*f;
	char	*data;
	unsigned long size;
    char* home = getenv("HOME");
    if (home == NULL) 
    {
        printf("Error: Unable to fetch home env! \n"); 
        exit(1);
    }
    char* path = "/Desktop/mw/";
    size_t len = strlen(home) + strlen(path) + 1;
    char* imgdb = malloc(len);
    if (imgdb == NULL) 
    {
        printf("Error: Unable to malloc() \n"); 
        exit(1);
    }
    strcpy(imgdb, home);
    strcat(imgdb, path);
    directory_exists_or_create(imgdb);

#if defined(BARCODE_ENABLED)
    /* 
     * Init barcode reader
     */
    init_barcode_com("/dev/ttyACM0");
#endif

	gp_log_add_func(GP_LOG_ERROR, errordumper, NULL);
	gp_camera_new(&camera);

	/* When I set GP_LOG_DEBUG instead of GP_LOG_ERROR above, I noticed that the
	 * init function seems to traverse the entire filesystem on the camera.  This
	 * is partly why it takes so long.
	 * (Marcus: the ptp2 driver does this by default currently.)
	 */
	printf("Camera init.  Takes about 10 seconds.\n");
	retval = gp_camera_init(camera, context);
	if (retval != GP_OK) {
		printf("  Retval of capture_to_file: %d\n", retval);
		exit (1);
	}

    printf(" ----------------\n");
    printf(" Sampler is ready \n");
    printf(" ----------------\n");
    printf("Usage : \n");
    printf("  ESC - Exit the program\n");
    printf("  i/I - Insert new product barcode manually\n");
#if defined(BARCODE_ENABLED)
    printf("  b/B - Insert new product barcode using barcode-scanner\n");
#endif

    char get_key;
    char exit_key = 0;
    char bcr_buf[128] = {0};
    int hemispheres_counts = 0;
    int rotar_steps = 0;

    do
    {
        get_key = getchar();

        switch (get_key)
        {
            // Gracefull Exit
            case _ESC_:
                exit_key = 1;
                break;

            // Barcode Mode    
            case 'b':
            case 'B':
#if defined(BARCODE_ENABLED)
                printf("ACTION: Scan the barcode on the product\n");
                if (read_from_barcode_reader(bcr_buf) < 0) {
                    printf("Error: Unable to read barcode input!\n");
                    printf("ACTION: Please enter manually:\n");
                    scanf("%128s", bcr_buf);
                }
#endif
                goto process;

            // Manual insert mode
            case 'i':
            case 'I':
                printf("ACTION: Type in the barcode and place it on the MellowSam plate.\n");
                scanf("%128s", bcr_buf);

process:

                press_enter();

                printf("ACTION: Shall we start? press return key.\n");
                press_enter();

                hemispheres_counts = 0;
                rotar_steps = 0;
            
                char product_filename[256] = {0};    
                strcpy(product_filename, imgdb);
                strcat(product_filename, bcr_buf);
                if (directory_exists_or_create(product_filename))
                {
                    printf("\n\n!!! ATTENTION: The product already exists !!!\n\n");
                    break;
                }

                while (hemispheres_counts < MAX_HEMISPHERES)
                {
                    while (rotar_steps < MAX_ROTAR_STEPS)
                    {
	                    capture_to_memory(camera, context, (const char**)&data, &size);

                        char fname[64] = {0};
                        char mk_filename[256] = {0};
                        strcpy(mk_filename, product_filename);
                        snprintf(fname, sizeof(fname), "/%d-%d.jpg", hemispheres_counts, rotar_steps);
                        strcat(mk_filename, fname); 
                        printf("file name %s\n", mk_filename);
	                    f = fopen(mk_filename, "wb");
	                    if (f) {
		                    retval = fwrite (data, size, 1, f);
		                    if (retval != size) {
			                    printf("  fwrite size %ld, written %d\n", size, retval);
		                    }
		                    fclose(f);
	                    } 
                        else 
                        {
		                    printf("  fopen *.jpg failed. %s\n", strerror(errno));
                        }
                        rotar_steps++;
                        usleep(1500*1000);
                    }

                    rotar_steps = 0;
                    hemispheres_counts++;

                    if (hemispheres_counts < MAX_HEMISPHERES)
                    {
                        printf("Flip the product and hit 'RETURN' key\n");
                        press_enter();      // This expect some input from user, thats it.
                        printf("Started capturing other hemisphere!\n");
                    } else {
                        printf("Sampling done for product barcode %s", bcr_buf);
                        break;
                    }
                }
                break;
        }
    } while (exit_key != 1);


    /* Release all the resources */
    printf("\nReleasing all the resources ... \n");
	gp_camera_exit(camera, context);
    free(imgdb);
#if defined(BARCODE_ENABLED)
    close_bcr();
#endif
    printf("Done.\n");
	return 0;
}
