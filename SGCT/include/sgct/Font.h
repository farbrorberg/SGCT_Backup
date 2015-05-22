/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _FREETYPE_FONT_H_
#define _FREETYPE_FONT_H_

#ifndef SGCT_DONT_USE_EXTERNAL
	#include <external/freetype/ftglyph.h>
#else
	#include <freetype/ftglyph.h>
#endif

#include <vector>
#include <string>

namespace sgct_text
{

/*!
Will ahandle font textures and rendering. Implementation is based on
<a href="http://nehe.gamedev.net/tutorial/freetype_fonts_in_opengl/24001/">Nehe's font tutorial for freetype</a>.
*/
class Font
{
public:
	Font( const std::string & fontName = std::string(), float height = 0.0f );
	~Font();

	void init( const std::string & fontName, unsigned int h );
	void clean();

	/*! Get the list base index */
	inline unsigned int getListBase() const { return mListBase; }

	/*! Get the vertex array id */
	inline unsigned int getVAO() const { return mVAO; }

	/*! Get the vertex buffer objects id */
	inline unsigned int getVBO() const { return mVBO; }

	/*! Get height of the font */
	inline float getHeight() const { return mHeight; }

	/*! Get the texture id's */
	inline const unsigned int * getTextures() const { return mTextures; }

	/*! Adds a glyph to the font */
	inline void AddGlyph( const FT_Glyph & glyph ){ mGlyphs.push_back( glyph ); }

	/*! Set the width of a character in the font */
	inline void setCharWidth( char c, float width ){ mCharWidths[ static_cast<size_t>(c) ] = width; }
	/*! Get the width of a character in the font */
	inline float getCharWidth( char c ) const { return mCharWidths[ static_cast<size_t>(c) ]; }


public:

	/*! Less then Font comparison operator */
	inline bool operator<( const Font & rhs ) const
	{ return mName.compare( rhs.mName ) < 0 || (mName.compare( rhs.mName ) == 0 && mHeight < rhs.mHeight); }

	/*! Equal to Font comparison operator */
	inline bool operator==( const Font & rhs ) const
	{ return mName.compare( rhs.mName ) == 0 && mHeight == rhs.mHeight; }

private:
	std::string mName;				// Holds the font name
	float mHeight;					// Holds the height of the font.
	unsigned int * mTextures;		// Holds the texture id's
	unsigned int mListBase;			// Holds the first display list id
	unsigned int mVBO;
	unsigned int mVAO;
	std::vector<FT_Glyph> mGlyphs;	// All glyphs needed by the font
	float * mCharWidths;
};

} // sgct

#endif
