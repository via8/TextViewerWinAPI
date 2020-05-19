#include "Error.h"

static char * errorMessages[ERR_UNKNOWN + 1] = {
    "no error",
    "command line argument not specified (name of processed file)",
    "NULL pointer passed",
    "error opening file",
    "unexpected end of file while reading",
    "error reading file",
    "not enough memory",
    "unknown error"
};

/**
 * Prints error message in chosen filestream.
 * IN:
 * @param output - stream to print into
 * @param errorType - type of error occured (enum ErrorType)
 * @param filename - name of file where error has occured
 * @param line - number of line in file where error has occured
 */
void PrintError(FILE * output, ErrorType errorType, char const * filename, int line) {
    if (output == NULL)
        output = stderr;
    if (filename == NULL)
        fprintf(output, "ERROR: %s\n", errorMessages[errorType]);
    else
        fprintf(output, "ERROR: %s\nFILE: %s\nLINE: %i\n", errorMessages[errorType], filename, line);
}
