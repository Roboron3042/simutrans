#include "simstring.h"
#include "../tpl/debug_helper.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>


static char thousand_sep = ',';
static char fraction_sep = '.';
static const char *large_number_string = "M";
static double large_number_factor = 1e99;	// off



// a single use number to string ...
char *ntos(int number, const char *format)
{
	static char tempstring[32];
	int r;

	if (format) {
		r = sprintf(tempstring, format, number);
	}
	else {
		r = sprintf(tempstring, "%d", number);
	}
	assert(r<16);

	return tempstring;
}




/**
 * Set thousand seperator, used in money_to_string and
 * number_to_string
 * @author Hj. Malthaner
 */
void set_thousand_sep(char c)
{
	thousand_sep = c;
}


/**
 * Set fraction seperator, used in money_to_string and
 * number_to_string
 * @author Hj. Malthaner
 */
void set_fraction_sep(char c)
{
	fraction_sep = c;
}


char get_fraction_sep(void)
{
	return fraction_sep;
}

void set_large_amout(const char *s, const double v)
{
	large_number_string = s;
	large_number_factor = v;
}


/**
 * Formats a money value. Uses thousand separator. Two digits precision.
 * Concludes format with $ sign. Buffer must be large enough, no checks
 * are made!
 * @author Hj. Malthaner
 */
void money_to_string(char * p, double f)
{
	char   tmp[128];
	char   *tp = tmp;
	int    i,l;

	if(  f>1000.0*large_number_factor  ) {
		sprintf(tp,"%.1f%s",large_number_string,f/large_number_factor);
	}
	else {
		sprintf(tp,"%.2f",f);
	}

	// Hajo: skip sign
	if(*tp == '-') {
		*p ++ = *tp++;
	}

	// Hajo: format string
	l = strchr(tp,'.') - tp;

	i = l % 3;

	if(i != 0) {
		memcpy(p, tp, i);
		p += i;
		*p++ = thousand_sep;
	}

	while(i < l) {
		*p++ = tp[i++];
		*p++ = tp[i++];
		*p++ = tp[i++];
		*p++ = thousand_sep;
	}
	--p;
	i = l+1;

	*p++ = fraction_sep;
	*p++ = tp[i++];
	*p++ = tp[i++];

	*p++ = '$';
	*p = 0;
}


void number_to_string(char * p, double f)
{
  char   tmp[128];
  char   *tp = tmp;
  int    i,l;

  sprintf(tp,"%.2f",f);

  // Hajo: skip sign
  if(*tp == '-') {
    *p ++ = *tp++;
  }

  // Hajo: format string
  l = strchr(tp,'.') - tp;

  i = l % 3;

  if(i != 0) {
    memcpy(p, tp, i);
    p += i;
    *p++ = thousand_sep;
  }

  while(i < l) {
    *p++ = tp[i++];
    *p++ = tp[i++];
    *p++ = tp[i++];
    *p++ = thousand_sep;
  }
  --p;
  i = l+1;

  *p = 0;
}



// copies a n into a single line and maximum 128 characters
// @author prissi
char *make_single_line_string(const char *in,int number_of_lines)
{
	static char buf[64];
	int pos;

	// skip leading whitespaces
	while(*in=='\n'  ||  *in==' ') {
		in++;
	}
	// start copying
	for(pos=0;  pos<62  &&  *in!=0  &&  number_of_lines>0;  ) {
		if((unsigned)(*in)>' ') {
			buf[pos++] = *in++;
		}
		else {
			// replace new lines by space
			while(*in=='\n'  ||  *in==' ') {
				if(*in=='\n') {
					number_of_lines--;
				}
				in++;
			}
			buf[pos++] = ' ';
		}
	}
	// trim trailing spaces
	while(pos>0  &&  buf[pos-1]==' ') {
		pos--;
	}
	// end mark!
	buf[pos] = 0;
	return buf;
}




/**
 * Terminated, length limited string copy. Copies at most
 * n characters. Terminates dest string always by 0.
 * @return dest
 * @author Hj. Malthaner
 */
char *tstrncpy(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n);
	dest[n-1] = '\0';

	return dest;
}


/**
 * Removes whitespace from the end of the string.
 * Modifies the argument!
 * @author Hj. Malthaner
 */
void rtrim(char * buf)
{
  int l = strlen(buf) - 1;

  while(l >= 0 && buf[l] > 0 && buf[l] <= 32) {
    buf[l--] = '\0';
  }
}


/**
 * Hands back a pointer to the first non-whitespace character
 * of the argument. The argument must be 0 terminated
 * @author Hj. Malthaner
 */
const char * ltrim(const char *p)
{
  while(*p != '\0' && *p > 0 && *p <= 32) {
    p ++;
  }

  return p;
}


int count_char(const char* str, const char c)
{
	int count = 0;
	while (*str != '\0') count += (*str++ == c);
	return count;
}
