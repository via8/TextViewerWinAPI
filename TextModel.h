#ifndef TEXTMODEL_H_INCLUDED
#define TEXTMODEL_H_INCLUDED

#include <windows.h>
#include <stdlib.h>
#include "Error.h"

typedef struct tag_StoredModel StoredModel;
typedef struct tag_DisplayedModel DisplayedModel;

typedef enum {
    VIEW_MODE_STANDARD,
    VIEW_MODE_WRAP
} ViewMode;

struct tag_DisplayedModel {
    // capacity of client area in SYSTEM_FIXED_FONT characters
    int capacityCharsX;
    int capacityCharsY;

    // size of client area
    int clientAreaX;
    int clientAreaY;

    // character metrics of SYSTEM_FIXED_FONT
    int charPixelsX;
    int charPixelsY;

    ViewMode viewMode;
    
    /* current position in text definition depends on view mode:

     * VIEW_MODE_STANDARD:
     * defined with number of the first visible line
     * and it's position to the right from the first symbol

     * VIEW_MODE_WRAP:
     * defined with the first visible symbol index in entire text file
     * and number of the line it belongs to */

    long firstLine;
    long firstSymbol;

    // scrollbar position calculation necessary info
    int scrollX;
    int scrollY;
    int scrollMaxX;
    int scrollMaxY;
    long linesNumberWrap;   // number of lines in wrap view mode
};

typedef struct {
    StoredModel * stored;
    DisplayedModel * displayed;
} TextModel;

ErrorType   BuildTextModel(TextModel * model, char const * inputFilename);
ErrorType RebuildTextModel(TextModel * model, char const * inputFilename);
char const * GetLineStandard(StoredModel const * stored, long lineNumber, long position, int capacityCharsX, long * lineLength);
char const * GetLineWrap(StoredModel const * stored, DisplayedModel const * displayed, long linesToSkip, long * lineLength, long * prevSymbol, long * prevLine);
long GetLineBeginning(StoredModel const * stored, long lineNumber);
long UpdateModelStandardX(StoredModel const * stored, DisplayedModel * displayed, int incrementX);
long UpdateModelStandardY(StoredModel const * stored, DisplayedModel * displayed, int incrementY);
long UpdateModelWrapY(StoredModel const * stored, DisplayedModel * displayed, long incrementY);
long scrollToIncrementX(StoredModel const * stored, DisplayedModel const * displayed, int scroll);
long scrollToIncrementY(StoredModel const * stored, DisplayedModel const * displayed, int scroll);
int countScrollPositionX(StoredModel const * stored, DisplayedModel const * displayed);
int countScrollPositionY(StoredModel const * stored, DisplayedModel const * displayed);
void SwitchMode(StoredModel const * stored, DisplayedModel * displayed, int viewMode);
void UpdateModelMetrics(HWND hWindow, StoredModel const * stored, DisplayedModel * displayed, int prevCapacityCharsX);
void SetInvalidRectagleX(DisplayedModel const * displayed, long incrementCharsX, RECT * rectangle);
void SetInvalidRectagleY(DisplayedModel const * displayed, long incrementCharsY, RECT * rectangle);
void DestroyTextModel(TextModel * textModel);

#endif // TEXTMODEL_H_INCLUDED
