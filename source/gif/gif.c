#include "gif.h"
 
#include <string.h>
#include <malloc.h>

const u16 gifIOff[] = { 0, 4, 2, 1 };
const u16 gifIJmp[] = { 8, 8, 4, 2 };

u32 gifHandlerLoadNextFrame(pGifHandler gifHandler) {
    GifRecordType gifRecType;
    unsigned char lineBuffer[256];
    unsigned char *Line;
    int i, j, k;
	u32 frmCount;
    
    do {
        if (DGifGetRecordType(gifHandler->gifInfo, &gifRecType) == GIF_ERROR) {
            u32 frmCount = gifHandler->frmCount;
            gifHandlerDestroy(gifHandler);
            return frmCount;
        }
        
        switch(gifRecType) {
            case TERMINATE_RECORD_TYPE:
                return gifHandler->frmCount;
            
            case EXTENSION_RECORD_TYPE: {
                    GifByteType* gifExt;
                    int    gifExtCode;
                    uint16 paletteEntry;
    
                    if (DGifGetExtension(gifHandler->gifInfo, &gifExtCode, &gifExt) == GIF_ERROR) {
                        u32 frmCount = gifHandler->frmCount;
                        gifHandlerDestroy(gifHandler);
                        return frmCount;
                    }
                    
                    switch(gifExtCode) {
                        case APPLICATION_EXT_FUNC_CODE:
                            if (memcmp(gifExt, "NETSCAPE2.0", 11))
                                gifHandler->loopCnt = ((u16*)++gifExt)[16];
                            break;
                        case GRAPHICS_EXT_FUNC_CODE:
                            gifHandler->gifInfo->Image.transCol = gifExt[4];
                            gifHandler->gifTiming = ((u16*)gifExt)[1] * 10;
                            
                                // Swapping transparent color and pal0
                            paletteEntry = gifHandler->gifPalette[0];
                            gifHandler->gifPalette[0] = gifHandler->gifPalette[gifHandler->gifInfo->Image.transCol];
                            gifHandler->gifPalette[gifHandler->gifInfo->Image.transCol] = paletteEntry;
                            break;
                        default:
                            break;
                    }
                    
                    while(gifExt != NULL) {
                        if (DGifGetExtensionNext(gifHandler->gifInfo, &gifExt) == GIF_ERROR) {
                            u32 frmCount = gifHandler->frmCount;
                            gifHandlerDestroy(gifHandler);
                            return frmCount;
                        }
                    }
                }
                break;
                
            case IMAGE_DESC_RECORD_TYPE: {
    
                    if (DGifGetImageDesc(gifHandler->gifInfo) == GIF_ERROR) {
                        u32 frmCount = gifHandler->frmCount;
                        gifHandlerDestroy(gifHandler);
                        return frmCount;
                    }
                    
                    int gifWidth  = gifHandler->dimX;
                    //int gifHeight = gifHandler->dimY; // unused right now
                    
                    int curBoxWidth  = gifHandler->gifInfo->Image.Width;
                    int curBoxHeight = gifHandler->gifInfo->Image.Height;
                    int curBoxTop    = gifHandler->gifInfo->Image.Top;
                    int curBoxLeft   = gifHandler->gifInfo->Image.Left;
                    
                    if (gifHandler->gifInfo->Image.Interlace) {
                        for (i=0; i<4; ++i) {
                            for (j=gifIOff[i]; j<curBoxHeight; j+=gifIJmp[i]) {
                                if(DGifGetLine(gifHandler->gifInfo, lineBuffer, curBoxWidth) == GIF_ERROR) {
                                    u32 frmCount = gifHandler->frmCount;
                                    gifHandlerDestroy(gifHandler);
                                    return frmCount;
                                }
                                
/*
                                // now let the buffer ignore any pixels with palette entry 0 and
                                // copy the rest over the existing image
                                Line = gifHandler->gifFrame + (i*gifWidth);
                                for (k=0; k<gifWidth; k++) {
                                    if (lineBuffer[k] != gifHandler->gifInfo->Image.transCol) {
                                        if (lineBuffer[k] == 0) // entry 0 and transparency are swapped
                                            Line[k] = gifHandler->gifInfo->Image.transCol;
                                        else
                                            Line[k] = lineBuffer[k];
                                    }
                                }
*/
                                // now let the buffer copy the entire frame (including transparency)
                                Line = gifHandler->gifFrame + (curBoxTop+i)*gifWidth + curBoxLeft;
                                for (k=0; k<curBoxWidth; k++) {
                                    if (lineBuffer[k] == gifHandler->gifInfo->Image.transCol)
                                        Line[k] = 0;
                                    else if (lineBuffer[k] == 0) // entry 0 and transparency are swapped
                                        Line[k] = gifHandler->gifInfo->Image.transCol;
                                    else
                                        Line[k] = lineBuffer[k];
                                }
                            }
                        }
                    } else {
                        for (i=0; i<curBoxHeight; ++i) {
                            if (DGifGetLine(gifHandler->gifInfo, lineBuffer, curBoxWidth) == GIF_ERROR) {
                                u32 frmCount = gifHandler->frmCount;
                                gifHandlerDestroy(gifHandler);
                                return frmCount;
                            }
                            
                            // now let the buffer ignore any pixels with palette entry 0 and
                            // copy the rest over the existing image
                            Line = gifHandler->gifFrame + (curBoxTop+i)*gifWidth + curBoxLeft;
                            for (k=0; k<curBoxWidth; k++) {
                                if (lineBuffer[k] != gifHandler->gifInfo->Image.transCol) {
                                    if (lineBuffer[k] == 0) // entry 0 and transparency are swapped
                                        Line[k] = gifHandler->gifInfo->Image.transCol;
                                    else
                                        Line[k] = lineBuffer[k];
                                }
                            }
                        }
                    }
                    
                        // Frame decoded
                    gifHandler->frmCount += 1;
                }
                break;
            default:
                break;
        }
    } while (gifRecType != IMAGE_DESC_RECORD_TYPE);
    
    return gifHandler->frmCount;
}

int gifReadFunc(GifFileType* gifInfo, GifByteType* buf, int count) {
    return fread(buf, sizeof(GifByteType), count, (FILE*)gifInfo->UserData);
}

pGifHandler gifHandlerLoad(FILE *gifFile, pGifHandler gifHandler) {
    unsigned int i;
    
    if (gifFile == NULL)
        return NULL;
    
    if (gifHandler != NULL)
        gifHandlerDestroy(gifHandler);
    
    gifHandler = (pGifHandler)malloc(sizeof(tGifHandler));
    if (!gifHandler)
        return NULL;
    
     GifFileType* gifInfo = DGifOpen((void*)gifFile, gifReadFunc);
    if (!gifInfo) {
        free(gifHandler);
        return NULL;
    }
    gifHandler->gifInfo = gifInfo;
    
    gifHandler->dimX = gifInfo->SWidth;
    gifHandler->dimY = gifInfo->SHeight;
    
    // Get GIF palette
    gifHandler->colCount = gifInfo->SColorMap->ColorCount; // Inc transparency!
    for(i=0; i<gifHandler->colCount; ++i) {
        GifColorType* pColor = &gifInfo->SColorMap->Colors[i];
        gifHandler->gifPalette[i] = RGB8(pColor->Red, pColor->Green, pColor->Blue);
    }
    
    // Pre-set variables
    gifHandler->loopCnt = 1;
    gifHandler->frmCount = 0;
    gifHandler->gifTiming = 0;
    
    memset(gifHandler->gifFrame, 0, 256*192);
    gifHandlerLoadNextFrame(gifHandler);
    
    return gifHandler;
}
 
void gifHandlerDestroy(pGifHandler gifHandler) {
    if (gifHandler == NULL)
        return;
    
    fclose((FILE*) gifHandler->gifInfo->UserData);
    
    DGifCloseFile(gifHandler->gifInfo);
    
    // Destroy GIF handler
    free(gifHandler);
}
