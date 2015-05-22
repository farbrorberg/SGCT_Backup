/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_SETTINGS
#define _SGCT_SETTINGS

#include <stdio.h>
#include <string>
#ifndef SGCT_DONT_USE_EXTERNAL
	#include "external/tinythread.h"
#else
	#include <tinythread.h>
#endif

namespace sgct
{

/*!
	This singleton class will hold global SGCT settings.
*/
class SGCTSettings
{
public:
	enum CapturePathIndex { Mono = 0, LeftStereo, RightStereo };
	enum DrawBufferType { Diffuse = 0, Diffuse_Normal, Diffuse_Position, Diffuse_Normal_Position };
	enum BufferFloatPrecision { Float_16Bit = 0, Float_32Bit };
    enum FisheyeMethod { FourFaceCube = 0, FiveFaceCube };

	/*! Get the SGCTSettings instance */
	static SGCTSettings * instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new SGCTSettings();
		}

		return mInstance;
	}

	/*! Destroy the SGCTSettings instance */
	static void destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	// ----------- set functions ---------------- //
	void setSwapInterval(int val);
	void setRefreshRateHint(int freq);
	void setUseDepthTexture(bool state);
	void setUseNormalTexture(bool state);
	void setUsePositionTexture(bool state);
	void setBufferFloatPrecision(BufferFloatPrecision bfp);
	void setUseFBO(bool state);
	void setNumberOfCaptureThreads(int count);
	void setPNGCompressionLevel(int level);
	void setJPEGQuality(int quality);
	void setCapturePath(std::string path, CapturePathIndex cpi = Mono);
	void appendCapturePath(std::string str, CapturePathIndex cpi = Mono);
	void setCaptureFormat(const char * format);
	void setFXAASubPixTrim(float val);
	void setFXAASubPixOffset(float val);
	void setOSDTextXOffset(float val);
	void setOSDTextYOffset(float val);
	void setOSDTextFontSize( int size );
	void setOSDTextFontName( std::string name );
	void setOSDTextFontPath( std::string path );
	void setDefaultNumberOfAASamples(int samples);
	void setDefaultFXAAState(bool state);
	void setForceGlTexImage2D(bool state);
	void setUsePBO(bool state);
	void setUseRLE(bool state);
	void setUseWarping(bool state);
    void setFisheyeMethod(FisheyeMethod fm);
	
	// ----------- get functions ---------------- //
	const char *		getCapturePath(CapturePathIndex cpi = Mono);
	int					getCaptureFormat();
	int					getSwapInterval();
	int					getRefreshRateHint();
	const int &			getOSDTextFontSize();
	const std::string &	getOSDTextFontName();
	const std::string &	getOSDTextFontPath();
	int					getBufferFloatPrecisionAsGLint();
	int					getDefaultNumberOfAASamples();
	bool				getDefaultFXAAState();
	bool				getForceGlTexImage2D();
	bool				getUsePBO();
	bool				getUseRLE();
	bool				getUseWarping();
    FisheyeMethod       getFisheyeMethod();
	int					getPNGCompressionLevel();
	int					getJPEGQuality();

	// ----------- inline functions ---------------- //
	//! Return true if depth buffer is rendered to texture
	inline bool		useDepthTexture() { return mUseDepthTexture; }
	//! Return true if normals are rendered to texture
	inline bool		useNormalTexture() { return mUseNormalTexture; }
	//! Return true if positions are rendered to texture
	inline bool		usePositionTexture() { return mUsePositionTexture; }
	//! Returns true if FBOs are used
	inline bool		useFBO() { return mUseFBO; }
	//! Get the number of capture threads (for screenshot recording)
	inline int		getNumberOfCaptureThreads() { return mNumberOfCaptureThreads; }
	//! The relative On-Screen-Display text x-offset in range [0, 1]
	inline float	getOSDTextXOffset() { return mOSDTextOffset[0]; }
	//! The relative On-Screen-Display text y-offset in range [0, 1]
	inline float	getOSDTextYOffset() { return mOSDTextOffset[1]; }
	/*! \returns the FXAA removal of sub-pixel aliasing */
	inline float	getFXAASubPixTrim() { return mFXAASubPixTrim; }
	/*! \returns the FXAA sub-pixel offset */
	inline float	getFXAASubPixOffset() { return mFXAASubPixOffset; }
	/*! \returns the current drawBufferType */
	inline DrawBufferType getCurrentDrawBufferType() { return mCurrentDrawBuffer; }

private:
	SGCTSettings();
	~SGCTSettings();

	void updateDrawBufferFlag();

	// Don't implement these, should give compile warning if used
	SGCTSettings( const SGCTSettings & settings );
	const SGCTSettings & operator=(const SGCTSettings & settings );

private:
	static SGCTSettings * mInstance;

	int mCaptureFormat;
	int mSwapInterval;
	int mRefreshRate;
	int mNumberOfCaptureThreads;
	int mPNGCompressionLevel;
	int mJPEGQuality;
	int mDefaultNumberOfAASamples;
	
	bool mUseDepthTexture;
	bool mUseNormalTexture;
	bool mUsePositionTexture;
	bool mUseFBO;
	bool mDefaultFXAA;
	bool mForceGlTexImage2D;
	bool mUsePBO;
	bool mUseRLE;
	bool mUseWarping;
	float mOSDTextOffset[2];
	float mFXAASubPixTrim;
	float mFXAASubPixOffset;

	std::string mCapturePath[3];

	//fontdata
	std::string mFontName;
	std::string mFontPath;
	int mFontSize;

	DrawBufferType mCurrentDrawBuffer;
	BufferFloatPrecision mCurrentBufferFloatPrecision;
    FisheyeMethod mFisheyeMethod;

	//mutex
	tthread::mutex mMutex;
};
}

#endif
