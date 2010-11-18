/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

static ssize_t convert_string (const char* in, size_t in_length, char* out, size_t out_size, char* from, char* to)
{
	ICONV_CONST char*	in_ptr = (ICONV_CONST char*) in;
	size_t			in_bytesleft = in_length;
	char*			out_ptr = out;
	size_t			out_bytesleft = out_size - 1;
	iconv_t			cd = (iconv_t) -1;
	ssize_t			res;

	cd = iconv_open (to, from);
	if (cd == (iconv_t) -1)
	{
		return -2;
	}

	res = iconv (cd, &in_ptr, &in_bytesleft, &out_ptr, &out_bytesleft);
	if (res < 0)
	{
		return -3;
	}

	iconv_close (cd);

	*out_ptr = '\0';

	return (out_ptr - out);
}

static ssize_t hexstr_to_ucs2char (const char* in, size_t in_length, char* out, size_t out_size)
{
	size_t i;
	size_t x;
	int hexval = 0;
	char buf[] = "  ";

	in_length = in_length / 2;

	if (out_size - 1 < in_length)
	{
		return -1;
	}

	for (i = 0, x = 0; i < in_length; i++)
	{
		memmove (buf, in + i * 2, 2);
		if (sscanf (buf, "%x", &hexval) != 1)
		{
			return -1;
		}
		out[x] = hexval;
		x++;
	}

	out[x] = '\0';

	return x;
}

static ssize_t ucs2char_to_hexstr (const char* in, size_t in_length, char* out, size_t out_size)
{
	size_t i;
	size_t x;
	char buf[] = "  ";

	if (out_size - 1 < in_length * 2)
	{
		return -1;
	}

	for (i = 0, x = 0; i < in_length; i++)
	{
		snprintf (buf,sizeof (buf),"%.2X", in[i]);
		memmove (out + x, buf, 2);
		x = x + 2;
	}

	out[x] = '\0';

	return x;
}

static ssize_t hexstr_ucs2_to_utf8 (const char* in, size_t in_length, char* out, size_t out_size)
{
	char	buf[out_size];
	ssize_t	res;

	if (out_size - 1 < in_length / 2)
	{
		return -1;
	}

	res = hexstr_to_ucs2char (in, in_length, buf, out_size);
	if (res < 0)
	{
		return res;
	}

	res = convert_string (buf, res, out, out_size, "UCS-2BE", "UTF-8");

	return res;
}

static ssize_t utf8_to_hexstr_ucs2 (const char* in, size_t in_length, char* out, size_t out_size)
{
	char	buf[out_size];
	ssize_t	res;

	if (out_size - 1 < in_length * 2)
	{
		return -1;
	}

	res = convert_string (in, in_length, buf, out_size, "UTF-8", "UCS-2BE");
	if (res < 0)
	{
		return res;
	}

	res = ucs2char_to_hexstr (buf, res, out, out_size);

	return res;
}

static ssize_t char_to_hexstr_7bit (const char* in, size_t in_length, char* out, size_t out_size)
{
	size_t		i;
	size_t		x;
	size_t		s;
	unsigned char	c;
	unsigned char	b;
	unsigned char	buf[] = { 0x0, 0x0, 0x0 };

	x = (in_length - in_length / 8) * 2;
	if (out_size - 1 < x)
	{
		return -1;
	}

	in_length--;
	for (i = 0, x = 0, s = 0; i < in_length; i++)
	{
		if (s == 7)
		{
			s = 0;
			continue;
		}

		c = in[i] >> s;
		b = in[i + 1] << (7 - s);
		c = c | b;
		s++;

		snprintf (buf, sizeof(buf), "%.2X", c);

		memmove (out + x, buf, 2);
		x = x + 2;
	}

	c = in[i] >> s;
	snprintf (buf, sizeof(buf), "%.2X", c);
	memmove (out + x, buf, 2);
	x = x + 2;

	out[x] = '\0';

	return x;
}

static ssize_t hexstr_7bit_to_char (const char* in, size_t in_length, char* out, size_t out_size)
{
	size_t		i;
	size_t		x;
	size_t		s;
	int		hexval;
	unsigned char	c;
	unsigned char	b;
	unsigned char	buf[] = { 0x0, 0x0, 0x0 };

	in_length = in_length / 2;
	x = in_length + in_length / 7;
	if (out_size - 1 < x)
	{
		return -1;
	}

	for (i = 0, x = 0, s = 1, b = 0; i < in_length; i++)
	{
		memmove (buf, in + i * 2, 2);
		if (sscanf (buf, "%x", &hexval) != 1)
		{
			return -1;
		}

		c = ((unsigned char) hexval) << s;
		c = (c >> 1) | b;
		b = ((unsigned char) hexval) >> (8 - s);

		out[x] = c;
		x++; s++;

		if (s == 8)
		{
			out[x] = b;
			s = 1; b = 0;
			x++;
		}
	}

	out[x] = '\0';

	return x;
}
