#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pigpio.h>

// gcc -Wall -o find_test find_test.c -lpigpio

int spi;
struct timespec ts = {0, 10 * 1000000};		// wait 10ms


int main(void)
{
	printf("Looking for GBA\n");

	if(gpioInitialise() < 0)
	{
		return 1;
	}

	spi = spiOpen(0, 50000, 3);

	if(spi < 0)
	{
		return 2;
	}

	char send[4];
	char recv[4];

	do
	{
		send[0] = 0x00;
		send[1] = 0x00;
		send[2] = 0x62;
		send[3] = 0x02;

		spiXfer(spi, send, recv, 4);

		printf("r:0x%02x%02x%02x%02x\n", recv[0], recv[1], recv[2], recv[3]);

		nanosleep(&ts, NULL);

	} while(recv[0] != 0x72 || recv[1] != 0x02 || recv[2] != 0x62 || recv[3] != 0x02);

	printf("Found GBA!\n");

	return 0;
}
