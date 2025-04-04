/**
 * @file gif.h
 * @brief GIF file loading and convertion.
 *
 * Description of gif.h
 *
 * @addtogroup GIF GIF handling
 * @{
 */
 
#ifndef __GIF_H__
#define __GIF_H__
 
#include <nds.h>
#include <stdio.h> 
 
#include "gif_lib.h"
 
// ----------------- Types		-----------------
 
/**
 * @struct tGifHandler
 * @brief GIF handler
 */
typedef struct 
{
	u32 dimX;               /**< GIF image width in pixels */
	u32 dimY;               /**< GIF image height in pixels */
	u32 frmCount;           /**< Number of frame currently loaded in */
	u16 loopCnt;            /**< GIF Loop count (0 for unlimited)*/
	u16 colCount;            /**< Number of colors the GIF posses, length of gifPalette */
	u8 gifFrame[256*192];   /**< GIF bitmap buffer */
	u16 gifPalette[256];    /**< GIF palette */
	u32 gifTiming;          /**< GIF Timing indexes per frame */
    
    GifFileType* gifInfo;
 
} tGifHandler, *pGifHandler;	
 
// ----------------- Functions	-----------------
 
/**
 * @fn pGifHandler gifHandlerLoad(const char* filename)
 * @param filename GIF File name to be loaded
 * @return Inited GIF Handler or NULL for error
 * @brief Fills the handler with GIF information
 */
pGifHandler gifHandlerLoad(FILE *gifFile, pGifHandler gifHandler);

/**
 * @fn void gifHandlerLoadNextFrame(pGifHandler gifHandler)
 * @param gifHandler GIF Handler to load in the next frame from
 * @return returns loaded in frame number
 * @brief Loads in the next frame of the GIF Handler
 * @note This ups the frmCount of the GIF Handler if a next frame was available
 */
u32 gifHandlerLoadNextFrame(pGifHandler gifHandler);
 
/**
 * @fn void gifHandlerDestroy(pGifHandler gifHandler)
 * @param gifHandler GIF Handler to be destroyed
 * @brief Removes all data used by the GIF handler
 * @note This also clears palette and bitmap(s)
 */
void gifHandlerDestroy(pGifHandler gifHandler);

#endif
 
/**
 * @}
 */
