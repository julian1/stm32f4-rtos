

#include <ctype.h>    // isdigit
#include <stdbool.h>    // true
#include <stdio.h>
#include <stdarg.h> // va_start etc

#include "assert.h"
#include "str.h"


char * indent_left(char *s, size_t sz, int indent, const char *string)
{
  // left indent, is pad to right, for field name
  snprintf(s, sz, "%-*s", indent, string );
  return s;
}



char * indent_right(char *s, size_t sz, int indent, const char *string)
{
  // right indent, is pad to left, for field value
  snprintf(s, sz, "%*s", indent, string);
  return s;
}



char * snprintf2(char *s, size_t sz, const char *format, ...)
{
  // same as snprintf but return the input buffer, as a convenience for caller
	va_list args;
	va_start(args, format);
	vsnprintf(s, sz, format, args);
	va_end(args);

  return s;
}



/*
  compiler issues warnings about callers of this func, if used in the same file. quite odd,
  i think because digit width takes priority over the input buffer length passed to snprintf
  so compiler is doing printf format checking on the inlined function.
*/

#if 0
char * format_float(char *s, size_t sz, int digits, double value)
{
  /*
    // eg. works
    printf("%0.*g\n",  5, 123.456789 );      // 123.46
    printf("%0.*g\n",  5, 12.3456789 );      // 12.346
    printf("%0.*g\n",  5, -12.3456789 );     // -12.346
  */

  ASSERT( digits < ((int)sz) - 2);  // basic sanity check ... TODO review...

  /*
    correct number of digits, but this doesn't add trailing 0...

  */
  snprintf(s, sz, "%0.*g",  digits, value);
  return s;
}
#endif

char * format_float(char *s, size_t sz, int digits, double value)
{
  /*
  from man snprintf
    %g The precision specifies the number of significant digits.  [...]
    Trailing zeros are removed from the fractional part of the result; a decimal
    point appears only if it is followed by at least one digit.  */


  // format
  size_t i = snprintf(s, sz, "%0.*g",  digits, value);

  // count how many digits we got
  int c = 0;
  for(char *p = s; *p; ++p) {
    if(isdigit((int)*p))
      ++c;
  }

  // check for decimal decimal place...
  bool dot = false;
  for(char *p = s; *p; ++p) {
    if(*p == '.')
      dot = true;
  }

  // maybe add dot
  if(!dot && i < (sz - 1)) {
    s[i++] = '.';
  }

  // maybe add trailing zeros that were ignored/dropped
  for(int k = c; k < digits && i < (sz - 1); ++k) {
    s[i++] = '0';
  }

  // add new sentinel
  ASSERT(i < sz);
  s[i++] = 0;
  return s;
}









char * format_bits(char *buf, size_t width, uint32_t value)
{
  // passing the buf, means can use more than once in printf expression. using separate bufs
  char *s = buf;

  for(int i = width - 1; i >= 0; --i) {
    *s++ = value & (1 << i) ? '1' : '0';
  }

  *s = 0;
  return buf;
}


