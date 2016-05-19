#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>

void main(void)
{
	printf("Looking for GBA\n");

	wiringPiSPISetupMode(0, 100000, 3);
	uint8_t buf[4];

	do
	{
		buf[0] = 0x00;
		buf[1] = 0x00;
		buf[2] = 0x62;
		buf[3] = 0x02;

		wiringPiSPIDataRW(0, &buf, 4);

		printf("r:0x%02x%02x%02x%02x\n", buf[0], buf[1], buf[2], buf[3]);
		usleep(10000);

	} while(buf[0] != 0x72 || buf[1] != 0x02 || buf[2] != 0x62 || buf[3] != 0x02);

	printf("Found GBA!\n");
}
