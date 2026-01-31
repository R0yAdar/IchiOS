#include "stdio.h"

#undef stdout
#undef stderr

FILE *stdout = (FILE *)1;
FILE *stderr = (FILE *)2;