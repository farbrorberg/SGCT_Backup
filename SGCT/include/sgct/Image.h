/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _IMAGE_H_
#define _IMAGE_H_

#ifndef SGCT_DONT_USE_EXTERNAL
#include "external/pngconf.h"
#else
#include <pngconf.h>
#endif

#include <string>

namespace sgct_core
{

class Image
{
public:
    enum ChannelType { Blue = 0, Green, Red, Alpha };
	enum FormatType { FORMAT_PNG = 0, FORMAT_JPEG, FORMAT_TGA, UNKNOWN_FORMAT };
    
	Image();
	~Image();

	bool allocateOrResizeData();
	bool load(std::string filename);
	bool loadPNG(std::string filename);
    bool loadPNG(unsigned char * data, int len);
	bool loadJPEG(std::string filename);
    bool loadJPEG(unsigned char * data, int len);
	bool save();
	bool savePNG(std::string filename, int compressionLevel = -1);
	bool savePNG(int compressionLevel = -1);
	bool saveJPEG(int quality = 100);
	bool saveTGA();
	void setFilename(std::string filename);

	unsigned char * getData();
	int getChannels() const;
	int getWidth() const;
	int getHeight() const;
	int getDataSize() const;
    unsigned char getSampleAt(int x, int y, ChannelType c);
    float getInterpolatedSampleAt(float x, float y, ChannelType c);

	void setDataPtr(unsigned char * dPtr);
	void setSize(int width, int height);
	void setChannels(int channels);
	inline const char * getFilename() { return mFilename.c_str(); }

private:
	void cleanup();
	bool allocateRowPtrs();
	FormatType getFormatType(const std::string & filename);
    bool isTGAPackageRLE(unsigned char * row, int pos);
    int getTGAPackageLength(unsigned char * row, int pos, bool rle);
    
private:
	bool mExternalData;
	int mChannels;
	int mSize_x;
	int mSize_y;
	int mDataSize;
	std::string mFilename;
	unsigned char * mData;
	png_bytep * mRowPtrs;
};

}

#endif

