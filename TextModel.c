#include "TextModel.h"
#include <string.h>
#include <limits.h>
#include <math.h>

struct tag_StoredModel {
    long fileSize;              // Size of processed file in bytes
    long linesNumber;           // Number of lines in the file (number of linebreaks symbols + 1)
    long maxLength;             // Length of the longest line in file
    long * lineBeginnings;      // Array of [linesNumber] indexes showing each line start point
    char * data;                // Buffer with processed file data
};

/**
 * Allocates memory for text model.
 * IN:
 * @param model - pointer to TextModel structure to allocate memory for
 * @param linesNumber - number of lines in initial file
 * 
 * OUT:
 * model->stored gets pointer to memory allocated
 * model->displayed gets pointer to memory allocated
 * model->stored->lineBeginnings gets pointer to memory allocated
 * @return TRUE if successed, FALSE else
 */
static BOOL AllocateTextModel(TextModel * model, long linesNumber) {
    model->stored = (StoredModel*)malloc(sizeof(StoredModel));
    if (model->stored == NULL)
        return FALSE;
    model->stored->lineBeginnings = (long*)malloc((linesNumber + 1) * sizeof(long));
    if (model->stored->lineBeginnings == NULL) {
        free(model->stored);
        return FALSE;
    }
    model->displayed = (DisplayedModel*)malloc(sizeof(DisplayedModel));
    if (model->displayed == NULL) {
        free(model->stored->lineBeginnings);
        free(model->stored);
        return FALSE;
    }
    return TRUE;
}

/**
 * Count number of lines of file in wrap view mode with specified size until number of specified line.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param capacityCharsX - capacity of chars of client area width
 * @param stopLine - number of line in initial file until which counting will continue
 * 
 * OUT:
 * @return number of lines in wrap mode
 */
static long CountLinesNumberWrap(StoredModel const * stored, int capacityCharsX, long stopLine) {
    int counter = 0;
    int i, j;

    for (i = 0; i < stopLine; ++i) {
        for (j = stored->lineBeginnings[i]; j < stored->lineBeginnings[i + 1]; j += capacityCharsX)
            ++counter;
    }

    return counter;
}

/**
 * Frees memory allocated for text model.
 * IN:
 * @param model - pointer to model structure of text file
 * 
 * OUT:
 * model->stored sets as NULL
 * model->displayed variables sets as NULL
 */
void DestroyTextModel(TextModel * model) {
    if (model == NULL)
        return;

    // destroy stored model
    if (model->stored != NULL) {
        if (model->stored->data != NULL)
            free(model->stored->data);
        if (model->stored->lineBeginnings != NULL)
            free(model->stored->lineBeginnings);
        free(model->stored);
    }

    // destroy displayed model
    if (model->displayed != NULL)
        free(model->displayed);
    
    model->stored = NULL;
    model->displayed = NULL;
}

/** 
 * Builds TextModel structure of file with specified name.
 * IN:
 * @param model - pointer to structure to save information in
 * @param inputFilename - name of file to process
 * 
 * OUT:
 * model->stored gets pointer to allocated memory
 * model->displayed gets pointer to allocated memory
 * fields of model->stored, model->displayed structures initialized with built text model parameters
 * @return code of error occured during building model (ERR_NO if successed)
 */
ErrorType BuildTextModel(TextModel * model, char const * inputFilename) {
    FILE * file = NULL;
    char * data = NULL;
    long fileSize;
    long linesNumber;
    long maxLength;
    long i, j;

    if (model == NULL) { // REMOVED 30/11/2019: || inputFilename == NULL) {
        PrintError(NULL, ERR_NULL_PTR, __FILE__, __LINE__);
        return ERR_NULL_PTR;
    }

    file = fopen(inputFilename, "rb");

    // REMOVED 30/11/2019:
    // if (file == NULL) {
    //     PrintError(NULL, ERR_OPEN_FILE, __FILE__, __LINE__);
    //     return ERR_OPEN_FILE;
    // }

    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        data = (char*)calloc(fileSize + 1, sizeof(char));
        if (data == NULL) {
            PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
            return ERR_NOMEM;
        }

        if (fread((void*)data, sizeof(char), fileSize, file) != fileSize) {
            if (feof(file)) {
                PrintError(NULL, ERR_EOF, __FILE__, __LINE__);
                return ERR_EOF;
            }
            PrintError(NULL, ERR_READ, __FILE__, __LINE__);
            return ERR_READ;
        }
        fclose(file);
    }
    // ADDED 30/11/2019: else build blank model
    else {
        fileSize = 0;
        data = (char*)calloc(fileSize + 1, sizeof(char));
        if (data == NULL) {
            PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
            return ERR_NOMEM;
        }
    }

    data[fileSize] = '\0';

    // start building model
    for (linesNumber = 1, i = 0; i < fileSize; ++i) {
        if (data[i] == '\n')
            linesNumber++;
    }

    // memory allocation
    if (!AllocateTextModel(model, linesNumber)) {
        PrintError(NULL, ERR_NOMEM, __FILE__, __LINE__);
        return ERR_NOMEM;
    }

    // fill structs field
    model->stored->data = data;
    model->stored->linesNumber = linesNumber;
    model->stored->fileSize = fileSize;

    model->stored->lineBeginnings[0] = 0;
    for (i = 0, j = 0; i < fileSize; ++i) {
        if (data[i] == '\n')
            model->stored->lineBeginnings[++j] = i + 1;     // after '\n' symbol
    }
    model->stored->lineBeginnings[linesNumber] = fileSize;  // special value to check end of file

    for (maxLength = 0, j = 0; j < linesNumber - 1; ++j) {
        if (maxLength < model->stored->lineBeginnings[j + 1] - model->stored->lineBeginnings[j])
            maxLength = model->stored->lineBeginnings[j + 1] - model->stored->lineBeginnings[j];
    }
    model->stored->maxLength = maxLength;

    // displayed model fields initialization
    // TODO: refactor with calloc
    model->displayed->capacityCharsX  = 0;
    model->displayed->capacityCharsY  = 0;
    model->displayed->clientAreaX     = 0;
    model->displayed->clientAreaY     = 0;
    model->displayed->charPixelsX     = 0;
    model->displayed->charPixelsY     = 0;
    model->displayed->viewMode        = VIEW_MODE_STANDARD;
    model->displayed->firstLine       = 0;
    model->displayed->firstSymbol     = 0;
    model->displayed->scrollX         = 0;
    model->displayed->scrollY         = 0;
    model->displayed->scrollMaxX      = 0;
    model->displayed->scrollMaxY      = 0;
    model->displayed->linesNumberWrap = 0;

    return ERR_NO;
}

/** 
 * Builds new TextModel structure of file with specified name and destroys previous one.
 * IN:
 * @param model - pointer to structure to save information in
 * @param inputFilename - name of file to process
 * 
 * OUT:
 * model->stored gets pointer to new allocated memory
 * model->displayed gets pointer to new allocated memory
 * fields of model->stored, model->displayed structures initialized with new built text model parameters
 * @return code of error occured during building model (ERR_NO if successed)
 */
ErrorType RebuildTextModel(TextModel * model, char const * inputFilename) {
    DisplayedModel tempDisplayed;
    ErrorType errorType;

    if (model == NULL) { // REMOVED || inputFilename == NULL) {
        PrintError(NULL, ERR_NULL_PTR, __FILE__, __LINE__);
        return ERR_NULL_PTR;
    }
    
    // save previous displayed model settings in local variable
    tempDisplayed = *model->displayed;

    // destroy previous model
    DestroyTextModel(model);

    // build new model
    errorType = BuildTextModel(model, inputFilename);
    if (errorType != ERR_NO)
        return errorType;
    
    // initialize new displayed model fields with previous settings
    model->displayed->capacityCharsX = tempDisplayed.capacityCharsX;
    model->displayed->capacityCharsY = tempDisplayed.capacityCharsY;
    model->displayed->charPixelsX    = tempDisplayed.charPixelsX;
    model->displayed->charPixelsY    = tempDisplayed.charPixelsY;
    model->displayed->clientAreaX    = tempDisplayed.clientAreaX;
    model->displayed->clientAreaY    = tempDisplayed.clientAreaY;
    model->displayed->viewMode       = tempDisplayed.viewMode;

    // set initial position in file
    model->displayed->firstLine      = 0;
    model->displayed->firstSymbol    = 0;
    model->displayed->scrollX        = 0;
    model->displayed->scrollY        = 0;

    return ERR_NO;
}

/**
 * Gives pointer to string for printing in standard view mode. Saves it's length in lineLength.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param lineNumber - number of line where to search substring
 * @param position - position of first symbol in line to print
 * @param capacityCharsX - capacity of chars of client area width
 * 
 * OUT:
 * @param lineLength - gets length of substring returned
 * @return pointer to desired substring (NULL if no string found)
 */
char const * GetLineStandard(StoredModel const * stored, long lineNumber, long position, int capacityCharsX, long * lineLength) {
    long firstSymbol;   // index of the first visible symbol in invalid region
    long tempLength;    // returned length of the line to output

    if (lineNumber >= stored->linesNumber)
        return NULL;

    firstSymbol = stored->lineBeginnings[lineNumber] + position;

    // set possible length of the line to output
    if (lineNumber == stored->linesNumber - 1)
        tempLength = stored->fileSize - firstSymbol;        // last line length
    else
        tempLength = stored->lineBeginnings[lineNumber + 1] - firstSymbol;

    if (lineLength != NULL)
       *lineLength = (tempLength > capacityCharsX) ?
                      capacityCharsX : max(0, tempLength);  // check if length is valid

    return &stored->data[firstSymbol];                      // return pointer to the string
}

/**
 * Gives pointer to string for printing in wrap view mode. Saves it's length in lineLength.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * @param linesToSkip - number of lines to skip counting from prevLine either firstLine of displayed model
 * @param prevLine - number of previous processed line in text
 * 
 * INOUT:
 * @param prevSymbol - number of symbol in text of previous processed line, gets index of returned substring beginning
 * @param prevLine - number of previous processed line in text, gets number of line which returned substring belongs to
 * 
 * OUT:
 * @param lineLength - gets length of substring returned
 * @return pointer to desired substring (NULL if no string found)
 */
char const * GetLineWrap(StoredModel const * stored, DisplayedModel const * displayed, long linesToSkip, long * lineLength, long * prevSymbol, long * prevLine) {
    long currLine   =   (prevLine == NULL) ? displayed->firstLine   : *prevLine;
    long currSymbol = (prevSymbol == NULL) ? displayed->firstSymbol : *prevSymbol;

    // skip lines till the first visible line of invalid rectangle
    while (linesToSkip != 0) {
        if (currSymbol +  displayed->capacityCharsX < stored->lineBeginnings[currLine + 1])
            currSymbol += displayed->capacityCharsX;
        else {
            currLine++;
            currSymbol = stored->lineBeginnings[currLine];
        }

        if (currSymbol >= stored->fileSize)
            return NULL;
        linesToSkip--;
    }

    // set valid length
    if (lineLength != NULL)
       *lineLength = min(stored->lineBeginnings[currLine + 1] - currSymbol, displayed->capacityCharsX);
    // set invalid rectangle's current visible line beginning
    if (prevSymbol != NULL)
       *prevSymbol = currSymbol;
    // set invalid rectangle's current visible line number
    if (prevLine != NULL)
       *prevLine = currLine;
    return &stored->data[currSymbol];    // return pointer to the string
}

/**
 * Updates displayed model firstSymbol field in standard mode 
 * according to received desired horizontal shift (in characters) of client area.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * @param incrementX - desired horizontal shift of client area
 * 
 * OUT:
 * displayed->firstSymbol gets position of new first visible symbol
 * @return actual possible horizontal shift of client area
 */
long UpdateModelStandardX(StoredModel const * stored, DisplayedModel * displayed, int incrementX) {
    long temp;
    if (incrementX < 0)
        incrementX = -min(displayed->firstSymbol, -incrementX);
    else {
        temp = stored->maxLength - displayed->firstSymbol - displayed->capacityCharsX;
        if (temp < 0)
            temp = 0;
        incrementX = min(temp, incrementX);
    }

    displayed->firstSymbol += incrementX;
    return incrementX;
}

/**
 * Updates displayed model firstLine field in standard mode
 * according to received desired vertical shift (in characters) of client area.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * @param incrementY - desired vertical shift of client area
 * 
 * OUT:
 * displayed->firstLine gets number of new first visible line
 * @return actual possible vertical shift of client area
 */
long UpdateModelStandardY(StoredModel const * stored, DisplayedModel * displayed, int incrementY) {
    long temp;
    if (incrementY < 0)
        incrementY = -min(displayed->firstLine, -incrementY);
    else {
        temp = stored->linesNumber - displayed->firstLine - displayed->capacityCharsY;
        if (temp < 0)
            temp = 0;
        incrementY = min(temp, incrementY);
    }

    displayed->firstLine += incrementY;
    return incrementY;
}

/**
 * Updates displayed model firstLine and firstSymbol fields in wrap mode 
 * according to received desired vertical shift (in characters) of client area.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * @param incrementY - desired vertical shift of client area
 * 
 * OUT:
 * displayed->firstSymbol gets index of new first visible symbol in text
 * displayed->firstLine gets number of new first visible line firstSymbol belongs to
 * @return actual possible vertical shift of client area
 */
long UpdateModelWrapY(StoredModel const * stored, DisplayedModel * displayed, long incrementY) {
    long remainingLineLength;       // length of the line which we're going to cut
    long shiftsLeft = incrementY;   // temp variable: shifts left to do

    if (incrementY > 0) {
        // if we shift up (incrementY > 0) client area then remainingLineLength equals:
        remainingLineLength = stored->lineBeginnings[displayed->firstLine + 1] -
                              displayed->firstSymbol;
        // then we try to cut into pieces our lines in the cycle until it's length allows to do so
        for (shiftsLeft = incrementY;
             // while there are shifts left to do and we still have lines to cut
             // (GetLineWrap returns NULL, if there are no lines left)
             shiftsLeft != 0 && GetLineWrap(stored, displayed, displayed->capacityCharsY, NULL, NULL, NULL) != NULL;
             shiftsLeft--) {
            // if it's possible to cut current line then decrease it's length in capacity (of characters) of client area width
            if (remainingLineLength >= displayed->capacityCharsX) {
                remainingLineLength -= displayed->capacityCharsX;
                displayed->firstSymbol += displayed->capacityCharsX;
            }
            // else update firstLine field (which means we go to the next line)
            else {
                displayed->firstSymbol += remainingLineLength;
                displayed->firstLine++;
                // get next line length
                remainingLineLength = stored->lineBeginnings[displayed->firstLine + 1] -
                                      displayed->firstSymbol;
            }
        }
    } else {
        // if we shift down (incrementY < 0) client area then remainingLineLength equals:
        remainingLineLength = displayed->firstSymbol -
                              stored->lineBeginnings[displayed->firstLine];
        // then we try to cut into pieces our lines in the cycle until it's length allows to do so
        
        // while there are shifts left to do and we still have lines to cut
        // (firstSymbol equals to the index of first visible symbol in entire file for wrap mode (see DisplayedModel declarataion))
        for (shiftsLeft = incrementY; shiftsLeft != 0 && displayed->firstSymbol > 0; ++shiftsLeft) {
            // if it's possible to cut current line then decrease it's length in capacity (of characters) of client area width
            if (remainingLineLength >= displayed->capacityCharsX) {
                remainingLineLength -= displayed->capacityCharsX;
                displayed->firstSymbol -= displayed->capacityCharsX;
            }
            // else update firstLine field (which means we go to the previous line) 
            else {
                // get next line length
                remainingLineLength = stored->lineBeginnings[displayed->firstLine] -
                                      stored->lineBeginnings[displayed->firstLine - 1];
                displayed->firstSymbol -= remainingLineLength % displayed->capacityCharsX;
                displayed->firstLine--;
            }
        }
    }

    return incrementY - shiftsLeft;    // return number of successfull shifts (signed)
}

/**
 * Receives horizontal scrollbar position 
 * and converts it into needed size of horizontal shift (in characters) to perform.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * @param scroll - horizontal scrollbar position from 0 to scrollMaxX of displayed model struct
 * 
 * OUT:
 * @return size of horizontal shift to perform
 */
long scrollToIncrementX(StoredModel const * stored, DisplayedModel const * displayed, int scroll) {
    double temp;
    if (displayed->viewMode != VIEW_MODE_STANDARD)
        return 0;
    temp  = (double)scroll / displayed->scrollMaxX;
    temp *= (stored->maxLength - displayed->capacityCharsX + 1);
    return (long)round(temp) - displayed->firstSymbol;
}

/**
 * Receives vertical scrollbar position 
 * and converts it into needed size of vertical shift (in characters) to perform.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * @param scroll - vertical scrollbar position from 0 to scrollMaxY of displayed model struct
 * 
 * OUT:
 * @return size of vertical shift to perform
 */
long scrollToIncrementY(StoredModel const * stored, DisplayedModel const * displayed, int scroll) {
    double temp = (double)scroll / displayed->scrollMaxY;
    switch (displayed->viewMode) {
    case VIEW_MODE_STANDARD:
        temp *= (stored->linesNumber - displayed->capacityCharsY + 1);
        return (long)round(temp) - displayed->firstLine;
    case VIEW_MODE_WRAP:
        temp *= (displayed->linesNumberWrap - displayed->capacityCharsY + 1);
        return (long)round(temp) - CountLinesNumberWrap(stored, displayed->capacityCharsX, displayed->firstLine);
    default:
        return 0;
    }
}

/**
 * Counts horizontal scrollbar position according to current position in text:
 * firstSymbol field of displayed model struct.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * 
 * OUT:
 * @return horizontal scrollbar position from 0 to scrollMaxX of displayed model struct
 */
int countScrollPositionX(StoredModel const * stored, DisplayedModel const * displayed) {
    double temp;
    if (displayed->viewMode != VIEW_MODE_STANDARD)
        return 0;
    temp  = (double)displayed->firstSymbol / (stored->maxLength - displayed->capacityCharsX + 1);
    temp *= displayed->scrollMaxX;

    if (temp < 0)
        return 0;
    else if (temp > displayed->scrollMaxX)
        return displayed->scrollMaxX;
    else
        return (int)round(temp);
}

/**
 * Counts vertical scrollbar position according to current position in text:
 * firstLine field of displayed model struct (for standard mode),
 * firstLine and linesNumberWrap fields of displayed model struct (for wrap mode).
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * 
 * OUT:
 * @return vertical scrollbar position from 0 to scrollMaxY of displayed model struct
 */
int countScrollPositionY(StoredModel const * stored, DisplayedModel const * displayed) {
    double temp;
    switch (displayed->viewMode) {
    case VIEW_MODE_STANDARD:
        temp  = (double)displayed->firstLine / (stored->linesNumber - displayed->capacityCharsY + 1);
        temp *= displayed->scrollMaxY;
        break;

    case VIEW_MODE_WRAP:
        temp  = (double)CountLinesNumberWrap(stored, displayed->capacityCharsX, displayed->firstLine) /
                (displayed->linesNumberWrap - displayed->capacityCharsY);
        temp *= displayed->scrollMaxY;
        break;
    default:
        return 0;
    }

    if (temp < 0)
        return 0;
    else if (temp > displayed->scrollMaxY)
        return displayed->scrollMaxY;
    else
        return (int)round(temp);
}

/** 
 * Updates displayed model fields (scrollMaxX, scrollMaxY, firstSymbol and linesNumberWrap) to relevant.
 * Sets relevant window's scrollbars ranges and positions.
 * IN:
 * @param hWindow - handler of window
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * @param prevCapacityCharsX - previous value needed to find out 
 * whether there is necessity to change firstSymbol and linesNumberWrap fields
 * 
 * OUT:
 * displayed->scrollMaxX gets new horizontal scrollbar range
 * displayed->scrollMaxY gets new vertical scrollbar range
 * displayed->firstSymbol may be changed to it's line beginning
 * displayed->linesNumberWrap may be recounted to new number of lines in wrap mode
 */
void UpdateModelMetrics(HWND hWindow, StoredModel const * stored, DisplayedModel * displayed, int prevCapacityCharsX) {
    long temp;

    temp = stored->maxLength - displayed->capacityCharsX + 1;
    if (temp < 0)
        temp = 0;
    displayed->scrollMaxX = min(SHRT_MAX, temp);

    if (displayed->viewMode == VIEW_MODE_STANDARD) {
        temp = stored->linesNumber - displayed->capacityCharsY + 1;
        if (temp < 0)
            temp = 0;
        displayed->scrollMaxY = min(SHRT_MAX, temp);
    }
    if (displayed->viewMode == VIEW_MODE_WRAP) {
        // for wrap mode it's not possible to change window width
        // without jumping to the line beginning
        if (displayed->capacityCharsX != prevCapacityCharsX) {
            displayed->firstSymbol = stored->lineBeginnings[displayed->firstLine];
            displayed->linesNumberWrap = CountLinesNumberWrap(stored, displayed->capacityCharsX, stored->linesNumber);
        }
        temp = displayed->linesNumberWrap - displayed->capacityCharsY + 1;
        if (temp < 0)
            temp = 0;
        displayed->scrollMaxY = min(SHRT_MAX, temp);
    }
    SetScrollRange(hWindow, SB_HORZ, 0, displayed->scrollMaxX, TRUE);
    SetScrollRange(hWindow, SB_VERT, 0, displayed->scrollMaxY, TRUE);
    SetScrollPos(hWindow, SB_HORZ, countScrollPositionX(stored, displayed), TRUE);
    SetScrollPos(hWindow, SB_VERT, countScrollPositionY(stored, displayed), TRUE);
}

/**
 * Switches text view mode.
 * IN:
 * @param stored - pointer to stored model structure of text file
 * @param displayed - pointer to displayed model structure of text file
 * @param viewMode - new view mode to set
 * 
 * OUT:
 * displayed->viewMode gets new view mode (see enum ViewMode)
 * displayed->firstSymbol gets new value dependently on view mode
 * displayed->scrollX gets 0 for wrap view mode
 */
void SwitchMode(StoredModel const * stored, DisplayedModel * displayed, int viewMode) {
    if (viewMode == VIEW_MODE_STANDARD) {
        displayed->viewMode = VIEW_MODE_STANDARD;
        displayed->firstSymbol = 0;
    }
    else if (viewMode == VIEW_MODE_WRAP) {
        displayed->viewMode  = VIEW_MODE_WRAP;
        displayed->firstSymbol = stored->lineBeginnings[displayed->firstLine];
        displayed->scrollX = 0;
    }
}


/**
 * Calculates borders of invalid rectangle according to received horizontal shift (in characters) of client area.
 * IN:
 * @param displayed - pointer to displayed model structure of text file
 * @param incrementCharsX - horizontal shift of client area
 * 
 * OUT:
 * @param rectangle - pointer to RECT struct, fills with invalid rectangel borders
 */
void SetInvalidRectagleX(DisplayedModel const * displayed, long incrementCharsX, RECT * rectangle) {
    if (abs(incrementCharsX) > displayed->capacityCharsX) {
        rectangle->left = 0;
        rectangle->right = displayed->charPixelsX * displayed->capacityCharsX;
    }
    else if (incrementCharsX > 0) {
        rectangle->left = displayed->charPixelsX * (displayed->capacityCharsX - incrementCharsX);
        rectangle->right = displayed->charPixelsX * displayed->capacityCharsX;
    } else {
        rectangle->left = 0;
        rectangle->right = displayed->charPixelsX * incrementCharsX;
    }
    rectangle->top = 0;
    rectangle->bottom = displayed->charPixelsY * displayed->capacityCharsY;
}

/**
 * Calculates borders of invalid rectangle according to received vertical shift (in characters) of client area.
 * IN:
 * @param displayed - pointer to displayed model structure of text file
 * @param incrementCharsX - vertical shift of client area
 * 
 * OUT:
 * @param rectangle - pointer to RECT struct, fills with invalid rectangel borders
 */
void SetInvalidRectagleY(DisplayedModel const * displayed, long incrementCharsY, RECT * rectangle) {
    if (abs(incrementCharsY) > displayed->capacityCharsY) {
        rectangle->top = 0;
        rectangle->bottom = displayed->charPixelsY * displayed->capacityCharsY;
    }
    else if (incrementCharsY > 0) {
        rectangle->top = displayed->charPixelsY * (displayed->capacityCharsY - incrementCharsY);
        rectangle->bottom = displayed->charPixelsY * displayed->capacityCharsY;
    } else {
        rectangle->top = 0;
        rectangle->bottom = displayed->charPixelsY * incrementCharsY;
    }
    rectangle->left = 0;
    rectangle->right = displayed->charPixelsX * displayed->capacityCharsX;
}
