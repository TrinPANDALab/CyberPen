#include <stdio.h>
#include <pigpio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>


#define POWER_CTL     0x2D


const char codeVersion[3] = "1.0";  // code version number

const int timeDefault = 5;  // default duration of data stream, seconds
const int freqDefault = 5;  // default sampling rate of data stream, Hz

const int freqMax = 3200;  // maximal allowed cmdline arg sampling rate of data stream, Hz
const int speedSPI = 2000000;  // SPI communication speed, bps
const int freqMaxSPI = 100000;  // maximal possible communication sampling rate through SPI, Hz (assumption)

//~ const int coldStartSamples = 2;  // number of samples to be read before outputting data to console (cold start delays)
//~ const double coldStartDelay = 0.1;  // time delay between cold start reads
//~ const double accConversion = 2 * 16.0 / 8192.0;  // +/- 16g range, 13-bit resolution
//~ const double tStatusReport = 1;  // time period of status report if data read to file, seconds

void printUsage() {
    printf( "adxl345spi (version %s) \n"
            "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
            "\n"
            "Usage: adxl345spi [OPTION]... \n"
            "Read data from ADXL345 accelerometer through SPI interface on Raspberry Pi.\n"
            "Online help, docs & bug reports: <https://github.com/nagimov/adxl345spi/>\n"
            "\n"
            "Mandatory arguments to long options are mandatory for short options too.\n"
            "  -s, --save FILE     save data to specified FILE (data printed to command-line\n"
            "                      output, if not specified)\n"
            "  -t, --time TIME     set the duration of data stream to TIME seconds\n"
            "                      (default: %i seconds) [integer]\n"
            "  -f, --freq FREQ     set the sampling rate of data stream to FREQ samples per\n"
            "                      second, 1 <= FREQ <= %i (default: %i Hz) [integer]\n"
            "\n"
            "Data is streamed in comma separated format, e. g.:\n"
            "  time,     x,     y,     z\n"
            "   0.0,  10.0,   0.0, -10.0\n"
            "   1.0,   5.0,  -5.0,  10.0\n"
            "   ...,   ...,   ...,   ...\n"
            "  time shows seconds elapsed since the first reading;\n"
            "  x, y and z show acceleration along x, y and z axis in fractions of <g>.\n"
            "\n"
            "Exit status:\n"
            "  0  if OK\n"
            "  1  if error occurred during data reading or wrong cmdline arguments.\n"
            "", codeVersion, timeDefault, freqMax, freqDefault);
}

int readBytes(int handle, char addr, char *data, int count) {
    char buff[2];
    buff[0] = (addr<<1)|1;
    buff[1] = 0x00;
    return spiXfer(handle, buff, data, count);
}

int writeBytes(int handle, char *data, int count) {
    data[0] = data[0]<<1;
    return spiWrite(handle, data, count);
}

int readFIFO(int handle, int* x, int* y, int* z){
        char data[10] = {0,0,0,0,0,0,0,0,0,0};
        readBytes(handle, 0x11, data, 10);
        *x = (int32_t)((((uint32_t)data[1])<<12)|(((uint32_t)data[2])<<4)|(((uint32_t)data[3])>>4));
        if (data[1]&0x80) *x = *x - (1<<20);
        *y = (int32_t)((((uint32_t)data[4])<<12)|(((uint32_t)data[5])<<4)|(((uint32_t)data[6])>>4));
        if (data[4]&0x80) *y = *y - (1<<20);
        *z = (int32_t)((((uint32_t)data[7])<<12)|(((uint32_t)data[8])<<4)|(((uint32_t)data[9])>>4));
        if (data[7]&0x80) *z = *z - (1<<20);
        return 0;
}


int main(int argc, char *argv[]) {
    //~ int i;
    int h0, h1;
    
    //Buffers to read and write stuffs to SPI
    char buff[2] = {0,0};
    char data[10] = {0,0,0,0,0,0,0,0,0,0};
    
    int32_t x, y, z;    
    
    // Initialize the board and connection
    if (gpioInitialise() < 0) {
        printf("Failed to initialize GPIO!");
        return 1;
    }
    
    h0 = spiOpen(0, speedSPI, 0);
    h1 = spiOpen(1, speedSPI, 0);
    
    if(h0 < 0){
        printf("H0 Open error");
        return 1;
    }
    if (h1 < 0){
        printf("H1 Open error");
        return 1;
    }
    
    // Initialize the accelerelometer
    buff[0] = POWER_CTL;
    buff[1] = 0x00;
    writeBytes(h0, buff, 2);
    buff[0] = POWER_CTL;
    buff[1] = 0x00;
    writeBytes(h1, buff, 2);
    
    char addr = 0x11;
    double calib = 1.0/52428.8;
    int status;
        
    for (int i = 0; i < 1000; i++)
    {        
        //~ readBytes(h0, 0x04, buff, 2);
        //~ status = buff[1];
        //~ printf("%x\n", status);
        //~ if(status == 1){
            //~ readFIFO(h0, &x, &y, &z);
            //~ printf("x = %f  y = %f z= %f\n", x*calib,y*calib,z*calib);
        //~ }
        //~ else if (status>1){
            //~ printf("Error %x\n", status);
            //~ readFIFO(h0, &x, &y, &z);
        //~ }
        
        readBytes(h0, 0x04, buff, 2);\
        status = buff[1];
        printf("%x\n",status);
        
        if(status == 1)
        {
            readBytes(h0, addr, data, 10);
            x = (int32_t)((((uint32_t)data[1])<<12)|(((uint32_t)data[2])<<4)|(((uint32_t)data[3])>>4));
            if (data[1]&0x80) x = x - (1<<20);
            y = (int32_t)((((uint32_t)data[4])<<12)|(((uint32_t)data[5])<<4)|(((uint32_t)data[6])>>4));
            if (data[4]&0x80) y = y - (1<<20);
            z = (int32_t)((((uint32_t)data[7])<<12)|(((uint32_t)data[8])<<4)|(((uint32_t)data[9])>>4));
            if (data[7]&0x80) z = z - (1<<20);
            printf("x = %f  y = %f z= %f\n", x*calib,y*calib,z*calib);
        }
        else
        {
            printf("Error %x\n", status);
            readBytes(h0, addr, data, 10);
        }
    }
    
    
    
    gpioTerminate();
    printf("Done\n");
    return 0;
}
