#include <stdio.h>

#include "types.h"
#include "cio.h"
#include <cstdlib>

void align_to_byte(nal_buffer_t * pnal_buffer) {
	if (pnal_buffer->bitpos != 8) {
		pnal_buffer->pos++;
		pnal_buffer->bitpos = 8;
	}
}

uint8 read_bit(nal_buffer_t * pnal_buffer) {
	if (pnal_buffer->bitpos == 0) {
		pnal_buffer->pos++;
		pnal_buffer->bitpos = 8;
	}

	if (pnal_buffer->pos >= pnal_buffer->posmax) {
		fprintf(stdout, "!!! nal buffer overrun !!!\n");
	}

	uint8 ret = pnal_buffer->data[pnal_buffer->pos] & (1 << (pnal_buffer->bitpos - 1));
	pnal_buffer->bitpos--;
	//fprintf(stdout, "\t\t%02x[%d] = %d\n", pnal_buffer->data[pnal_buffer->pos], pnal_buffer->bitpos, ret);
	fprintf(stdout, "%d", ret > 0 ? 1 : 0);
	return ret > 0 ? 1 : 0;
}

uint32 read_bits(nal_buffer_t * pnal_buffer, int nbits) {
	uint32 ret = 0;
	for (int i = 0; i < nbits; i++) {
		ret = (ret << 1) | read_bit(pnal_buffer);
	}
	return ret;
}

uint64 read_bits64(nal_buffer_t * pnal_buffer, int nbits) {
	uint64 ret = 0;
	for (int i = 0; i < nbits; i++) {
		ret = (ret << 1) | read_bit(pnal_buffer);
	}
	return ret;
}

uint32 read_uev(nal_buffer_t * pnal_buffer) { // read exp-golomb code
	fprintf(stdout, "[");
	int zero_leading_bits = -1;
	uint8 b = 0;
	for (b = 0; !b; zero_leading_bits++) {
		b = read_bit(pnal_buffer);
	}
	uint32 ret = (1 << zero_leading_bits) - 1 + read_bits(pnal_buffer, zero_leading_bits);
	fprintf(stdout, "]");
	return ret;
}

sint32 read_sev(nal_buffer_t * pnal_buffer) { // read signed exp-golomb code
	uint32 val = read_uev(pnal_buffer);
	sint32 ret = 0;
	if (val > 0) {
		ret = ((val % 2) > 0 ? 1 : -1) * CEIL(val, 2);
	}
	return ret;

}

int write_bit(nal_buffer_t * pnal_buffer, uint8 b)
{
	if (pnal_buffer->bitpos == 0) {
		pnal_buffer->pos++;
		pnal_buffer->bitpos = 8;
	}

	if (pnal_buffer->pos >= pnal_buffer->posmax) {
		fprintf(stdout, "!!! nal buffer overrun !!!\n");
	}
	
	pnal_buffer->data[pnal_buffer->pos] |= (b & 1) << (pnal_buffer->bitpos -1);
	pnal_buffer->bitpos--;
	//fprintf(stdout, "\t\t%02x[%d] = %d\n", nal_buffer.data[nal_buffer.pos], nal_buffer.bitpos, ret);
	//fprintf(stdout, "%d", ret > 0 ? 1 : 0);
	return 1;
}


int write_bits(nal_buffer_t * pnal_buffer, void * v , int nbits)
{
	if (nbits > 0 && nbits <= 8)
	{
		uint8 b = 0;
		for (int i = nbits; i > 0; i++)
		{
			b = (*((uint8 *)v) >> (i - 1)) & 1;
			write_bit(pnal_buffer, b);
		}
		return nbits;
	}
	if (nbits <= 16)
	{
		uint16 b = 0;
		for (int i = nbits; i > 0; i++)
		{
			b = (*((uint16 *)v) >> (i - 1)) & 1;
			write_bit(pnal_buffer, b);
		}
		return nbits;
	}
	if (nbits <= 32)
	{
		uint32 b = 0;
		for (int i = nbits; i > 0; i++)
		{
			b = (*((uint32 *)v) >> (i - 1)) & 1;
			write_bit(pnal_buffer, b);
		}
		return nbits;
	}
	if (nbits <= 64)
	{
		uint64 b = 0;
		for (int i = nbits; i > 0; i++)
		{
			b = (*((uint64 *)v) >> (i - 1)) & 1;
			write_bit(pnal_buffer, b);
		}
		return nbits;
	}
	fprintf(stdout, "!!! can't write more then 64 bits !!!\n");
	return 0;
}

int write_uev(nal_buffer_t * pnal_buffer, uint32 num) //write exp-golomb code
{
	uint32 absV = num;
	uint8 stopLoop = 0;
	int k = 0;
	int i = 0;
	do
		if (absV >= (1 << k))
		{
			write_bit(pnal_buffer, 1);
			i++;
			absV = absV - (1 << k);
			k++;
		}
		else
		{
			write_bit(pnal_buffer, 0);
			i++;
			while (k--)
			{
				write_bit(pnal_buffer, (absV >> k) & 1);
				i++;
			}

			stopLoop = 1;
		}
	while (!stopLoop);
	return i;
}

int write_sev(nal_buffer_t * pnal_buffer, sint32 b)
{
	int ret = 0;
	if (b < 0)
	{
		ret = write_uev(pnal_buffer, -2*b);
	}
	else
	{
		ret = write_uev(pnal_buffer, 2*b - 1);
	}
	return ret;
}



