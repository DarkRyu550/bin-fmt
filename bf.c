#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

char *program_name;
void
usage()
{
	fprintf(stderr, "usage: %s [-o output] [input]\n", program_name);
}

int
main(int argc, char **argv)
{
	FILE *in;
	FILE *out;
	char *ifname;
	char *ofname;
	char state;			/* Machine state.					*/
	char token;			/* Current token.					*/
	char byte;			/* Byte being worked on.			*/
	char fill;			/* Byte used for filling.			*/
	char sum;			/* Sum of the last written bytes.	*/
	char check;			/* Value to be checked against sum.	*/
	long room;			/* Number of free bits.				*/
	unsigned long line;	/* Current line number.				*/
	unsigned long head;	/* Position of the RW head.			*/
	unsigned long len;	/* Length of the file.				*/
	unsigned int i, j;	

	program_name = argv[0];
	in = NULL;
	out = NULL;
	ifname = "-";
	ofname = "-";
	for(i = 1; i < (unsigned int)argc; ++i)
	{
		#define ARG(x, y) if((x) >= (unsigned int)argc) { usage(); exit(1); } else { y; }
		if(strcmp("-o", argv[i]) == 0)
		{
			ARG(++i, ofname = argv[i]);
		}
		else
		{
			ifname = argv[i];
		}
		#undef ARG
	}

	if(strcmp("-", ifname) == 0)
		in = stdin;
	else
		in = fopen(ifname, "r");
	
	if(strcmp("-", ofname) == 0)
		out = stdout;
	else
		out = fopen(ofname, "w");

	if(in == NULL)
	{
		fprintf(stderr, "cannot open input file %s: %s\n", 
			ifname, strerror(errno));
		return 1;
	}
	if(out == NULL)
	{
		fprintf(stderr, "cannot open output file %s: %s\n",
			ofname, strerror(errno));
		return 1;
	}

	byte  = 0;
	line  = 1;
	head  = 0;
	len   = 0;
	i     = 0;
	fill  = 0;
	sum   = 0;
	check = 0;
	while((token = fgetc(in)) != EOF)
	{
		/* fprintf(stderr, "got token: %c\n", token); */
		switch(token)
		{
		case '0':
		case '1':
			if(state == 'q')
			{
				byte <<= 1;
				byte |= (token - '0');

				i += 1;
				goto write;
			}
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if(state == 'w')
			{
				byte <<= 4;
				byte |= (token - '0');

				i += 4;
				goto write;
			}
			else if(state == '.')
			{
				head <<= 4;
				head |= (token - '0');
				room -= 4;
			}
			else if(state == 'l')
			{
				fill <<= 4;
				fill |= (token - '0');
				room -= 4;
			}
			else if(state == ':')
			{
				check <<= 4;
				check |= (token - '0');
				room -= 4;
			}
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if(state == 'w')
			{
				byte <<= 4;
				byte |= (token - 'a' + 0xa);
				
				i += 4;
				goto write;
			}
			else if(state == '.')
			{
				head <<= 4;
				head |= (token - 'a' + 0xa);
				room -= 4;
			}
			else if(state == 'l')
			{
				fill <<= 4;
				fill |= (token - 'a' + 0xa);
				room -= 4;
			}
			else if(state == ':')
			{
				check <<= 4;
				check |= (token - 'a' + 0xa);
				room -= 4;
			}
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if(state == 'w')
			{
				byte <<= 4;
				byte |= (token - 'A' + 0xa);

				i += 4;
				goto write;
			}
			else if(state == '.')
			{
				head <<= 4;
				head |= (token - 'A' + 0xa);
				room -= 4;
			}
			else if(state == 'l')
			{
				fill <<= 4;
				fill |= (token - 'A' + 0xa);
				room -= 4;
			}
			else if(state == ':')
			{
				check <<= 4;
				check |= (token - 'A' + 0xa);
				room -= 4;
			}
			break;
		write:
			if(i % 8 == 0)
			{
				fwrite(&byte, 1, 1, out);
				if(++head >= len)
					++len;
				sum += byte;
			}
			break;
		case 'w':
			/* Write hex bytes. */
			if(state == ';')
				break;

			if(state == '^')
			{
				/* Clear the cell register. */
				byte = 0;
				i += i % 8;
				break;
			}

			state = 'w';
			break;
		case '.':
			/* Move the RW head. */
			if(state == ';')
				break;
			state = '.';
			head  = 0;
			room  = sizeof(head) * 8;
			break;
		case 'l':
			/* Change the fillinh byte. */
			if(state == ';')
				break;
			state = 'l';
			fill  = 0;
			room  = sizeof(fill) * 8;
			break;
		case ':':
			/* Check sum. */
			if(state == ';')
				break;

			if(state == '^')
			{
				/* Clear the sum register. */
				sum = 0;
				break;
			}

			state = ':';
			check = 0;
			room  = sizeof(check) * 8;
		case 'q':
			/* Write bits. */
			if(state == ';')
				break;
			state = 'q';
			break;
		case ';':
			/* Enter a comment. */
			state = ';';
			break;
		case '>':
			/* Seek to the end. */
			if(state == ';')
				break;

			state = '>';
			if(len != 0)
				head = len - 1;
			break;
		case '^':
			/* Clear register. */
			if(state == ';')
				break;

			state = '^';
			break;
		case '\r':
		case '\n':
			/* Reset state. */
			if(state == '.' || state == '>')
			{
				if(head >= len)
				{
					for(j = 0; j < head - len; ++j)
						fwrite(&fill, 1, 1, out);
					len = head + 1;
				}
				else if(fseek(out, head, SEEK_SET) != 0)
				{
					fprintf(stderr, "error: cannot seek to %lu on line %lu: %s",
						head, line, strerror(errno));
					exit(1);
				}
			}
			else if(state == ':')
			{
				if((unsigned char) -sum != check)
				{
					fprintf(stderr, "checksum %hhx does not match sum %hhx\n",
						check, (unsigned char) -sum);
					exit(1);
				}
				sum = 0;
				check = 0;
			}

			state = 0;
			++line;
		}

		if(room < 0)
		{
			if(state == '.')
				fprintf(stderr, 
					"warning: head position overflow on line %lu\n", line);
			else if(state == 'f')
				fprintf(stderr,
					"warning: fill byte overflow on line %lu\n", line);
		}
	}

	/* Flush the cell register if needed. */
	if(i % 8 != 0)
		fwrite(&byte, 1, 1, out);

	fflush(out);
	return 0;
}

