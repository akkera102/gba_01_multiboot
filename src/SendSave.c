#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pigpio.h>

// gcc -Wall -o SendSave SendSave.c -lpigpio

//---------------------------------------------------------------------------
uint32_t Spi32(uint32_t w);
uint32_t Spi32Print(uint32_t w, char* msg);
void     Spi32WaitPrint(uint32_t w, uint32_t comp, char* msg);

void     CmdPrint(uint32_t cnt);
void     CmdPut(uint32_t chr);
void     CmdFopen(uint32_t len);
void     CmdFseek(void);
void     CmdFwrite(void);
void     CmdFclose(void);
void     CmdFread(void);
void     CmdFtell(void);

//---------------------------------------------------------------------------
int spi;
struct timespec ts1 = {0, 10 * 1000000};		// wait 10ms
struct timespec ts2 = {0,  2 * 1000000};		// wait  2ms

FILE* fpSave;

//---------------------------------------------------------------------------
int main(void)
{
	FILE *fp = fopen("SendSave_mb.gba", "rb");

	if(fp == NULL)
	{
		printf("Err: Can't open file\n");
		return 1;
	}

	fseek(fp, 0L, SEEK_END);
	long fsize = (ftell(fp) + 0x0f) & 0xfffffff0;

	if(fsize > 0x40000)
	{
		printf("Err: Max file size 256kB\n");
		return 2;
	}

	fseek(fp, 0L, SEEK_SET);
	long fcnt = 0;

	uint32_t r, w, w2;
	uint32_t i, bit;

	if(gpioInitialise() < 0)
	{
		return 3;
	}

	spi = spiOpen(0, 100000, 3);

	if(spi < 0)
	{
		return 4;
	}

	Spi32WaitPrint(0x00006202, 0x72026202, "Looking for GBA");

	r = Spi32Print(0x00006202, "Found GBA");
	r = Spi32Print(0x00006102, "Recognition OK");

	printf("                       ; Send Header\n");
	for(i=0; i<=0x5f; i++)
	{
		w = getc(fp);
		w = getc(fp) << 8 | w;
		fcnt += 2;

		r = Spi32(w);
	}

	r = Spi32Print(0x00006200, "Transfer of header data complete");
	r = Spi32Print(0x00006202, "Exchange master/slave info again");

	r = Spi32Print(0x000063d1, "Send palette data");
	r = Spi32Print(0x000063d1, "Send palette data, receive 0x73hh****");  

	uint32_t m = ((r & 0x00ff0000) >>  8) + 0xffff00d1;
	uint32_t h = ((r & 0x00ff0000) >> 16) + 0xf;

	r = Spi32Print((((r >> 16) + 0xf) & 0xff) | 0x00006400, "Send handshake data");
	r = Spi32Print((fsize - 0x190) / 4, "Send length info, receive seed 0x**cc****");

	uint32_t f = (((r & 0x00ff0000) >> 8) + h) | 0xffff0000;
	uint32_t c = 0x0000c387;


	printf("                       ; Send encrypted data\n");

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
		Spi32(w2 ^ ((~(0x02000000 + fcnt)) + 1) ^m ^0x43202f2f);

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

	Spi32WaitPrint(0x00000065, 0x00750065, "Wait for GBA to respond with CRC");

	r = Spi32Print(0x00000066, "GBA ready with CRC");
	r = Spi32Print(c,          "Let's exchange CRC!");

//	printf("CRC ...hope they match!\n");
	printf("MulitBoot done\n\n");


	// init
	do
	{
		nanosleep(&ts1, NULL);
		r = Spi32(0);

	} while(r != 0x50525406);


	// select cmd
	for(;;)
	{
		switch(r & 0xffffff00)
		{
		// PRINT_CMD
		case 0x50525400:
			CmdPrint(r & 0xff);
			break;

		// DPUTC_CMD
		case 0x44505400:
			CmdPut(r & 0xff);
			break;

		// GETCH_CMD
		case 0x47544300:
			// no USE
			break;

		// FOPEN_CMD
		case 0x4F504E00:
			CmdFopen(r & 0xff);
			break;

		// FSEEK_CMD
		case 0x46534B00:
			CmdFseek();
			break;

		// FWRITE_CMD
		case 0x46575200:
			CmdFwrite();
			break;

		// FCLOSE_CMD
		case 0x434C5300:
			CmdFclose();
			break;

		// FREAD_CMD
		case 0x46524400:
			CmdFread();
			break;

		// FTELL_CMD
		case 0x46544C00:
			CmdFtell();
			break;

		default:
//			printf("%08x\n", r);
			break;
		}

		nanosleep(&ts1, NULL);
		r = Spi32(0);
	}
}
//---------------------------------------------------------------------------
void CmdPrint(uint32_t cnt)
{
	uint32_t i, r;
	char c;

	for(i=0; i<cnt; i++)
	{
		if(i % 4 == 0)
		{
			r = Spi32(0);
			nanosleep(&ts2, NULL);
		}

		c = r & 0xff;

		if((c >= 0x20 && c <= 0x7E) || c == 0x0D || c == 0x0A)
		{
			printf("%c", c);
		}

		r >>= 8;
	}
}
//---------------------------------------------------------------------------
void CmdPut(uint32_t chr)
{
	printf("%c", chr);
	nanosleep(&ts1, NULL);
}
//---------------------------------------------------------------------------
void CmdFopen(uint32_t len)
{
	char fname[80];
	char ftype[3];

	memset(fname, 0x00, sizeof(fname));
	memset(ftype, 0x00, sizeof(ftype));

	uint32_t i, r;

	for(i=0; i<len; i++)
	{
		if(i % 4 == 0)
		{
			r = Spi32(0);
		}

		fname[i] = r & 0xff;
		r >>= 8;
	}

	r = Spi32(0);
	ftype[0] = (r     ) & 0xff;
	ftype[1] = (r >> 8) & 0xff;

//	printf("fopen %s %s\n", fname, ftype);
	fpSave = fopen(fname, ftype);

	Spi32(0);
}
//---------------------------------------------------------------------------
void CmdFseek(void)
{
	uint32_t offset = Spi32(0);
	uint32_t origin = Spi32(0);

//	printf("fseek %d %d\n", offset, origin);
	fseek(fpSave, offset, origin);
}
//---------------------------------------------------------------------------
void CmdFwrite(void)
{
//	printf("fwrite START\n");

	uint32_t i, r;

	uint32_t size  = Spi32(0);
	uint32_t count = Spi32(0);

	for(i=0; i<size*count; i++)
	{
		if(i % 4 == 0)
		{
			r = Spi32(0);
			nanosleep(&ts2, NULL);
		}

		fputc(r & 0xff, fpSave);
		r >>= 8;
	}

//	printf("fwrite END %d\n", size*count);
}
//---------------------------------------------------------------------------
void CmdFclose(void)
{
//	printf("fclose\n");
	fclose(fpSave);
}
//---------------------------------------------------------------------------
void CmdFread(void)
{
	uint32_t i, r;
	uint32_t d1, d2, d3, d4;

	uint32_t size  = Spi32(0);
	uint32_t count = Spi32(0);

	for(i=0; i<size*count/4; i++)
	{
		d1 = fgetc(fpSave);
		d2 = fgetc(fpSave);
		d3 = fgetc(fpSave);
		d4 = fgetc(fpSave);

		r = 0;

		r += d4;
		r <<= 8;
		r += d3;
		r <<= 8;
		r += d2;
		r <<= 8;
		r += d1;

		Spi32(r);
		nanosleep(&ts2, NULL);
	}

//	printf("fread %d %d\n", size, count);
}
//---------------------------------------------------------------------------
void CmdFtell(void)
{
	fseek(fpSave, 0, SEEK_END);
	Spi32(ftell(fpSave));

//	printf("ftell %d\n", ftell(fpSave));
}
//---------------------------------------------------------------------------
uint32_t Spi32(uint32_t w)
{
	char send[4];
	char recv[4];

	send[3] = (w & 0x000000ff);
	send[2] = (w & 0x0000ff00) >>  8;
	send[1] = (w & 0x00ff0000) >> 16;
	send[0] = (w & 0xff000000) >> 24;

	spiXfer(spi, send, recv, 4);

	uint32_t ret = 0;

	ret += ((unsigned char)recv[0]) << 24;
	ret += ((unsigned char)recv[1]) << 16;
	ret += ((unsigned char)recv[2]) <<  8;
	ret += ((unsigned char)recv[3]);

	return ret;
}
//---------------------------------------------------------------------------
uint32_t Spi32Print(uint32_t w, char* msg)
{
	uint32_t r = Spi32(w);

	printf("0x%08x 0x%08x  ; %s\n", r, w, msg);
	return  r;
}
//---------------------------------------------------------------------------
void Spi32WaitPrint(uint32_t w, uint32_t comp, char* msg)
{
	printf("%s 0x%08x\n", msg, comp);
	uint32_t r;

	do
	{
		r = Spi32(w);


	} while(r != comp);
}
