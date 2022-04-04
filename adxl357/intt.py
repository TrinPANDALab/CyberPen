import os, time, spidev
import RPi.GPIO as GPIO

# Constants
BUTTON_DRDY0 = 36
POWER_CTL = 0x2D
calib = 1.0/52428.8

#Setup Board
GPIO.setmode(GPIO.BOARD)

# Interrupt Setup
GPIO.setup(BUTTON_DRDY0, GPIO.IN, pull_up_down = GPIO.PUD_UP)

#Setup SPI
spi = spidev.SpiDev()
spi.open(0,1)
spi.max_speed_hz = 9760000
spi.mode = 0b00


def activate0():
	spi.xfer([POWER_CTL<<1, 0])
	
def writeBytes(addr, data):
	spi.xfer3([addr<<1] + data)
	
def readByte(addr, count):
	data = spi.xfer3([(addr<<1)|1]+([0]*count))
	return data[1:]
	
def readFIFO(i):
	ADDR = [0]*10;
	ADDR[0] = (0x11<<1)|1
	data = spi.xfer3(ADDR)
	
	x = (data[1]<<12)|(data[2]<<4)|(data[3]>>4);
	if(data[1]&0x80): x = x - (1<<20)
	y = (data[4]<<12)|(data[5]<<4)|(data[6]>>4);
	if(data[4]&0x80): y = y - (1<<20)
	z = (data[7]<<12)|(data[8]<<4)|(data[9]>>4);
	if(data[7]&0x80): z = z - (1<<20)
	
	return (x*calib,y*calib,z*calib)


	
def cleanupp():
	spi.close()
	GPIO.cleanup()

		
def intt():
	i = 0
	while True:
		GPIO.wait_for_edge(BUTTON_DRDY0, GPIO.FALLING)
		# ~ i = i + 1
		# ~ print(i)
		print(readFIFO(0))
		print(hex(readByte(0x04,1)[0]))
	
	
if __name__ == "__main__":
	# ~ activate0()
	# ~ intt()
	# ~ print(readFIFO(0))
	writeBytes(0x2D,[0])
	intt()
	print(hex(readByte(0x2,1)[0]))
	cleanupp()
