/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is SniffURI.
 *
 * The Initial Developer of the Original Code is
 * Erik van der Poel <erik@vanderpoel.org>.
 * Portions created by the Initial Developer are Copyright (C) 1998-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Bruce Robson <bns_robson@hotmail.com>
 *
 * ***** END LICENSE BLOCK ***** */

#include "all.h"

#define GULP 2048

struct Input
{
	unsigned long		readAlloc;
	const unsigned char	*readBuf;
	const unsigned char	*readBufPtr;
	const unsigned char	*readBufEnd;
	const unsigned char	*readBufMarkBegin;
	const unsigned char	*readBufMarkEnd;
	unsigned long		streamSize;
};

static Input *
inputAlloc(void)
{
	Input	*input;

	input = calloc(sizeof(Input), 1);
	if (!input)
	{
		fprintf(stderr, "cannot calloc Input\n");
		exit(0);
	}

	return input;
}

static Input *
readInit(void)
{
	Input	*input;

	input = inputAlloc();
	input->readAlloc = 1;
	input->readBuf = calloc(input->readAlloc + 1, 1);
	if (!input->readBuf)
	{
		fprintf(stderr, "cannot calloc readBuf\n");
		exit(0);
	}

	return input;
}

Input *
readStream(int fd, unsigned char *url)
{
	int		bytesRead;
	fd_set		fdset;
	Input		*input;
	int		offset;
	int		ret;
	unsigned long	streamSize;
	struct timeval	timeout;

	input = readInit();

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);

	timeout.tv_sec = 5 * 60;
	timeout.tv_usec = 0;

	offset = 0;
	streamSize = 0;
	while (1)
	{
		ret = select(fd + 1, &fdset, NULL, NULL, &timeout);
		if (!ret)
		{
			fprintf(stderr, "readStream: select timed out: %s\n",
				url);
			streamSize = 0;
			break;
		}
		else if (ret == -1)
		{
			perror("select");
			streamSize = 0;
			break;
		}
		if (offset + GULP > input->readAlloc)
		{
			input->readAlloc = offset + GULP;
			input->readBuf = realloc((void *) input->readBuf,
				input->readAlloc + 1);
			if (!input->readBuf)
			{
				fprintf(stderr, "cannot realloc readBuf %ld\n",
					input->readAlloc + 1);
				streamSize = 0;
				break;
			}
		}
		bytesRead = recv(fd, (void *) (input->readBuf + offset), GULP,
			0);
		if (bytesRead <= 0)
		{
			break;
		}
		else if (bytesRead > GULP)
		{
			/* should not happen */
			streamSize = 0;
			break;
		}
		else
		{
			offset += bytesRead;
			streamSize += bytesRead;
		}
	}

	((unsigned char *) input->readBuf)[streamSize] = 0;

	input->readBufPtr = input->readBuf;
	input->readBufEnd = input->readBuf + streamSize;
	input->streamSize = streamSize;
	input->readBufMarkEnd = input->readBuf;

	return input;
}

Input *
readAvailableBytes(int fd)
{
	int	bytesRead;
	Input	*input;

	input = inputAlloc();
	input->readAlloc = 10240;
	input->readBuf = calloc(input->readAlloc + 1, 1);
	if (!input->readBuf)
	{
		fprintf(stderr, "cannot calloc readBuf\n");
		exit(0);
	}
	input->readBufPtr = input->readBuf;
	input->readBufEnd = input->readBuf;
	input->readBufMarkEnd = input->readBuf;
	bytesRead = recv(fd, (void *) input->readBuf, input->readAlloc, 0);
	if (bytesRead < 0)
	{
		perror("recv");
		return input;
	}
	else if (bytesRead == input->readAlloc)
	{
		fprintf(stderr, "readBuf too small\n");
	}

	((unsigned char *) input->readBuf)[bytesRead] = 0;

	input->readBufEnd = input->readBuf + bytesRead;
	input->streamSize = bytesRead;

	return input;
}

void
inputFree(Input *input)
{
	free((char *) input->readBuf);
	free(input);
}

unsigned short
getByte(Input *input)
{
	if (input->readBufPtr >= input->readBufEnd)
	{
		input->readBufPtr++;
		return 256;
	}

	return *input->readBufPtr++;
}

void
unGetByte(Input *input)
{
	if (input->readBufPtr > input->readBuf)
	{
		input->readBufPtr--;
	}
}

const unsigned char *
current(Input *input)
{
	return input->readBufPtr;
}

void
set(Input *input, const unsigned char *pointer)
{
	input->readBufPtr = (unsigned char *) pointer;
}

unsigned long
inputLength(Input *input)
{
	return input->streamSize;
}

unsigned char *
copyMemory(Input *input, unsigned long *len)
{
	unsigned char	*ret;

	*len = input->readBufMarkEnd - input->readBufMarkBegin;
	ret = malloc(*len);
	if (!ret)
	{
		fprintf(stderr, "cannot calloc block\n");
		exit(0);
	}
	memcpy(ret, input->readBufMarkBegin, *len);

	return ret;
}

unsigned char *
copy(Input *input)
{
	return copySizedString(input->readBufMarkBegin,
		input->readBufMarkEnd - input->readBufMarkBegin);
}

unsigned char *
copyLower(Input *input)
{
	return lowerCase(copySizedString(input->readBufMarkBegin,
		input->readBufMarkEnd - input->readBufMarkBegin));
}

unsigned short
trimTrailingWhiteSpace(Input *input)
{
	unsigned char	c;

	input->readBufPtr -= 2;
	do
	{
		c = *input->readBufPtr--;
	} while
	(
		(c == ' ') ||
		(c == '\t') ||
		(c == '\r') ||
		(c == '\n')
	);
	input->readBufPtr += 2;

	return *input->readBufPtr++;
}

void
mark(Input *input, int offset)
{
	input->readBufMarkBegin = input->readBufMarkEnd;
	input->readBufMarkEnd = input->readBufPtr + offset;
}
