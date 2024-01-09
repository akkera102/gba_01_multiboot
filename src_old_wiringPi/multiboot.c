#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>

uint32_t WriteSPI32NoDebug(uint32_t w)
{
	uint8_t buf[4];

	buf[3] = (w & 0x000000ff);
	buf[2] = (w & 0x0000ff00) >>  8;
	buf[1] = (w & 0x00ff0000) >> 16;
	buf[0] = (w & 0xff000000) >> 24;

	wiringPiSPIDataRW(0, &buf, 4);

	uint32_t r = 0;

	r += buf[0] << 24;
	r += buf[1] << 16;
	r += buf[2] <<  8;
	r += buf[3];

	return r;
}

uint32_t WriteSPI32(uint32_t w, char* msg)
{
	uint32_t r = WriteSPI32NoDebug(w);

	printf("0x%08x 0x%08x  ; %s\n", r, w, msg);
	return  r;
}

void WaitSPI32(uint32_t w, uint32_t comp, char* msg)
{
	printf("%s 0x%08x\n", msg, comp);
	uint32_t r;

	do
	{
		r = WriteSPI32NoDebug(w);
		usleep(10000);

	} while(r != comp);
}

void main(void)
{
	FILE *fp = fopen("multiboot_mb.gba", "rb");
	if(fp == NULL)
	{
		printf("Err: Can't open file\n");
		return;
	}

	fseek(fp, 0L, SEEK_END);
	long fsize = (ftell(fp) + 0x0f) & 0xfffffff0;

	if(fsize > 0x40000)
	{
		printf("Err: Max file size 256kB\n");
		return;
	}

	fseek(fp, 0L, SEEK_SET);
	long fcnt = 0;

	uint32_t r, w, w2;
	uint32_t i, bit;

	wiringPiSPISetupMode(0, 100000, 3);

	WaitSPI32(0x00006202, 0x72026202, "Looking for GBA");

	r = WriteSPI32(0x00006202, "Found GBA");
	r = WriteSPI32(0x00006102, "Recognition OK");

	printf("Send Header(NoDebug)\n");
	for(i=0; i<=0x5f; i++)
	{
		w = getc(fp);
		w = getc(fp) << 8 | w;
		fcnt += 2;

		r = WriteSPI32NoDebug(w);
	}

	r = WriteSPI32(0x00006200, "Transfer of header data complete");
	r = WriteSPI32(0x00006202, "Exchange master/slave info again");

	r = WriteSPI32(0x000063d1, "Send palette data");
	r = WriteSPI32(0x000063d1, "Send palette data, receive 0x73hh****");  

	uint32_t m = ((r & 0x00ff0000) >>  8) + 0xffff00d1;
	uint32_t h = ((r & 0x00ff0000) >> 16) + 0xf;

	r = WriteSPI32((((r >> 16) + 0xf) & 0xff) | 0x00006400, "Send handshake data");
	r = WriteSPI32((fsize - 0x190) / 4, "Send length info, receive seed 0x**cc****");

	uint32_t f = (((r & 0x00ff0000) >> 8) + h) | 0xffff0000;
	uint32_t c = 0x0000c387;


	printf("Send encrypted data(NoDebug)\n");

	while(fcnt < fsize)
	{
		w = getc(fp);
		w = getc(fp) <<  8 | w;
		w = getc(fp) << 16 | w;
		w = getc(fp) << 24 | w;

		w2 = w;

		for(bit=0; bit<32; bit++)
		{
			if((c ^ w) & 0x01)
			{
				c = (c >> 1) ^ 0x0000c37b;
			}
			else
			{
				c = c >> 1;
			}

			w = w >> 1;
		}

		m = (0x6f646573 * m) + 1;
		WriteSPI32NoDebug(w2 ^ ((~(0x02000000 + fcnt)) + 1) ^m ^0x43202f2f);

		fcnt = fcnt + 4;
	}
	fclose(fp);

	for(bit=0; bit<32; bit++)
	{
		if((c ^ f) & 0x01)
		{
			c =( c >> 1) ^ 0x0000c37b;
		}
		else
		{
			c = c >> 1;
		}

		f = f >> 1;
	}

	WaitSPI32(0x00000065, 0x00750065, "Wait for GBA to respond with CRC");

	r = WriteSPI32(0x00000066, "GBA ready with CRC");
	r = WriteSPI32(c,          "Let's exchange CRC!");

	printf("CRC ...hope they match!\n");
	printf("MulitBoot done\n");
}
