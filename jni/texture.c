/* Copyright (c) <2012>, Intel Corporation
*
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met:
*
* - Redistributions of source code must retain the above copyright notice, 
*   this list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright notice, 
*   this list of conditions and the following disclaimer in the documentation 
*   and/or other materials provided with the distribution.
* - Neither the name of Intel Corporation nor the names of its contributors 
*   may be used to endorse or promote products derived from this software 
*   without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
* POSSIBILITY OF SUCH DAMAGE.
*
*/

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include "ktx.h"
#include "ktxint.h"

#include "file.h"
#include "texture.h"
#include "stb_image.h"

// Make sure all compression formats are defined
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG   0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG   0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG  0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG  0x8C03

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3


///////////////////////////////////////////////////////////////////////////////////////////////////
// Debugging helper functions
#define  Log(...)  __android_log_print( ANDROID_LOG_INFO, "TextureLoader", __VA_ARGS__ )
#define  LogError(...)  __android_log_print( ANDROID_LOG_ERROR, "TextureLoader", __VA_ARGS__ )

static void CheckGlError( const char* pFunctionName ) 
{
    GLint error = glGetError();
    if( error != GL_NO_ERROR ) 
    {
        LogError( "%s returned glError 0x%x\n", pFunctionName, error );
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Check if ETC is supported (by hardware)
int IsETCSupported()
{
    int count = 0;
    extern int GLEW_OES_compressed_ETC1_RGB8_texture;
    GLEW_OES_compressed_ETC1_RGB8_texture = 0;

    glGetIntegerv( GL_NUM_COMPRESSED_TEXTURE_FORMATS, &count );
    if( count > 0 )
    {
        GLint* formats = (GLint*)calloc( count, sizeof(GLint) );

        glGetIntegerv( GL_COMPRESSED_TEXTURE_FORMATS, formats );

        int index;
        for( index = 0; index < count; index++ )
        {
            switch( formats[index] )
            {
                case GL_ETC1_RGB8_OES:
                    GLEW_OES_compressed_ETC1_RGB8_texture = 1;
                    break;
            }
        }

        free( formats );
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Check if ETC2 is supported (by hardware)
int IsETC2Supported()
{
    // ETC2 is a standard feature in OpenGL ES 3.0 so it's always supported

    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Check if PVRTC is supported
int IsPVRTCSupported()
{
    int PVRTCSupported = -1;
    
    // Determine if we have support for PVRTC
    const GLubyte* pExtensions = glGetString( GL_EXTENSIONS );
    PVRTCSupported = strstr( (char*)pExtensions, "GL_IMG_texture_compression_pvrtc" ) != NULL;
    
    return PVRTCSupported;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Check if S3TC is supported
int IsS3TCSupported()
{
    int S3TCSupported = -1;
    
    // Determine if we have support for PVRTC
    const GLubyte* pExtensions = glGetString( GL_EXTENSIONS );
    S3TCSupported = strstr( (char*)pExtensions, "GL_EXT_texture_compression_s3tc" ) != NULL;
    
    return S3TCSupported;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Loads a PNG texture and returns a handle
//
// PNG loading code provided as public domain by Sean Barrett (http://nothings.org/)
GLuint LoadTexturePNG( const char* TextureFileName )
{   
    // Load Texture File
    char* pFileData = NULL;
    unsigned int fileSize = 0;
    
    ReadFile( TextureFileName, &pFileData, &fileSize );
    
    int width, height, numComponents;
    unsigned char* pData = stbi_load_from_memory( (unsigned char*)pFileData, fileSize, &width, &height, &numComponents, 0 );

    // Generate handle
    GLuint handle;
    glGenTextures( 1, &handle );
    
    // Bind the texture
    glBindTexture( GL_TEXTURE_2D, handle );
    
    // Set filtering mode for 2D textures (using mipmaps with bilinear filtering)
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
    
    // Determine the format
    GLenum format;
    
    switch( numComponents )
    {
        case 1:
        {
            // Gray
            format = GL_LUMINANCE;
            break;
        }
        case 2: 
        {
            // Gray and Alpha
            format = GL_LUMINANCE_ALPHA;
            break;
        }
        case 3: 
        {
            // RGB
            format = GL_RGB;
            break;
        }
        case 4: 
        {
            // RGBA
            format = GL_RGBA;
            break;
        }
        default: 
        {
            // Unknown format
            assert(0);
            return 0;
        }
    }
    
    // Initialize the texture
    glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pData);
    CheckGlError( "glTexImage2D" );
    
    // Generate mipmaps - to have better quality control and decrease load times, this should be done offline
    glGenerateMipmap( GL_TEXTURE_2D );
    CheckGlError( "glGenerateMipmap" );

    // clean up
    free( pFileData );
    free( pData );
    
    // Return handle
    return handle;  
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Loads a ETC texture and returns a handle
//
// KTX file defined at http://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
// This uses the KTX/ETC library provided by the Khronos Group (see libktx for details)
GLuint LoadTextureETC_KTX( const char* TextureFileName )
{    
    // Read/Load Texture File
    char* pData = NULL;
    unsigned int fileSize = 0;
    
    ReadFile( TextureFileName, &pData, &fileSize );
    
    // Generate handle & Load Texture
    GLuint handle = 0;
    GLenum target;
    GLboolean mipmapped;
        
    KTX_error_code result = ktxLoadTextureM( pData, fileSize, &handle, &target, NULL, &mipmapped, NULL, NULL, NULL );
        
    if( result != KTX_SUCCESS )
    {
        LogError( "KTXLib couldn't load texture %s. Error: %d", TextureFileName, result );
        return 0;
    }
 
    // Bind the texture
    glBindTexture( target, handle );
        
    // Set filtering mode for 2D textures (bilinear filtering)
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    if( mipmapped )
    {
        // Use mipmaps with bilinear filtering
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
    }

    // clean up
    free( pData );
    
    // Return handle
    return handle;  
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Loads a PVRTC texture and returns a handle (mipmap support included)
//
// Extension defined here: http://www.khronos.org/registry/gles/extensions/IMG/IMG_texture_compression_pvrtc.txt
// File format defined here: http://www.imgtec.net/powervr/insider/docs/PVR%20File%20Format.Specification.1.0.11.External.pdf
typedef struct
{
    unsigned int       mVersion;            
    unsigned int       mFlags;          
    unsigned long long mPixelFormat;        
    unsigned int       mColourSpace;        
    unsigned int       mChannelType;        
    unsigned int       mHeight;         
    unsigned int       mWidth;          
    unsigned int       mDepth;          
    unsigned int       mNumSurfaces;        
    unsigned int       mNumFaces;       
    unsigned int       mMipmapCount;     
    unsigned int       mMetaDataSize;   
} __attribute__((packed)) PVRHeaderV3;  // Header must be packed because of the 64-bit mPixelFormat (ARM will pad 4 extra bytes to make it aligned)

GLuint LoadTexturePVRTC( const char* TextureFileName )
{
    // Load the texture file
    char* pData = NULL;
    unsigned int fileSize = 0;
    
    ReadFile( TextureFileName, &pData, &fileSize );
    
    // Grab the header
    PVRHeaderV3* pHeader = (PVRHeaderV3*)pData;
    
    // Generate handle
    GLuint handle;
    glGenTextures( 1, &handle );
    
    // Bind the texture
    glBindTexture( GL_TEXTURE_2D, handle );
    
    // Set filtering mode for 2D textures (bilenear filtering)
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    if( pHeader->mMipmapCount > 1 )
    {
        // Use mipmaps with bilinear filtering
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
    }
    
    // Determine the format
    GLenum format;
    GLuint bitsPerPixel;

    switch( pHeader->mPixelFormat )
    {
        case 0:
        {
            // PVRTC 2bpp RGB
            format = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            bitsPerPixel = 2;
            break;
        }
        case 1: 
        {
            // PVRTC 2bpp RGBA
            format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            bitsPerPixel = 2;
            break;
        }
        case 2: 
        {
            // PVRTC 4bpp RGB
            format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            bitsPerPixel = 4;
            break;
        }
        case 3: 
        {
            // PVRTC 4bpp RGBA
            format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            bitsPerPixel = 4;
            break;
        }
        default: 
        {
            // Unknown format
            assert(0);
            return 0;
        }
    } 
    
    // Initialize the texture
    unsigned int offset = sizeof(PVRHeaderV3) + pHeader->mMetaDataSize;
    unsigned int mipWidth = pHeader->mWidth;
    unsigned int mipHeight = pHeader->mHeight;

    unsigned int mip = 0;
    do
    {
        // Determine size (width * height * bbp/8)
        // pixelDataSize must be at least two blocks (4x4 pixels for 4bpp, 8x4 pixels for 2bpp), so min size is 32  
        unsigned int pixelDataSize = ( mipWidth * mipHeight * bitsPerPixel ) >> 3;
        pixelDataSize = (pixelDataSize < 32) ? 32 : pixelDataSize;

        // Upload texture data for this mip
        glCompressedTexImage2D(GL_TEXTURE_2D, mip, format, mipWidth, mipHeight, 0, pixelDataSize, pData + offset);
        CheckGlError("glCompressedTexImage2D");
    
        // Next mips is half the size (divide by 2) with a min of 1
        mipWidth = mipWidth >> 1;
        mipWidth = ( mipWidth == 0 ) ? 1 : mipWidth;

        mipHeight = mipHeight >> 1;
        mipHeight = ( mipHeight == 0 ) ? 1 : mipHeight; 

        // Move to next mip
        offset += pixelDataSize;
        mip++;
    } while(mip < pHeader->mMipmapCount);

    // clean up
    free( pData );
  
    // Return handle
    return handle;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Loads a S3TC texture and returns a handle
//
// Extension defined here: http://oss.sgi.com/projects/ogl-sample/registry/EXT/texture_compression_s3tc.txt
// File format defined here: http://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx

typedef struct 
{ 
    unsigned int mSize;
    unsigned int mFlags;
    unsigned int mFourCC;
    unsigned int mRGBBitCount;
    unsigned int mRedBitMask;
    unsigned int mGreenBitMask;
    unsigned int mBlueBitMask;
    unsigned int mAlphaBitMask;
} DDSPixelFormat;

typedef struct 
{
    char           mFileType[4];
    unsigned int   mSize;
    unsigned int   mFlags;
    unsigned int   mHeight;
    unsigned int   mWidth;
    unsigned int   mPitchOrLinearSize;
    unsigned int   mDepth;
    unsigned int   mMipMapCount;
    unsigned int   mReserved1[11];
    DDSPixelFormat mPixelFormat;
    unsigned int   mCaps;
    unsigned int   mCaps2;
    unsigned int   mCaps3;
    unsigned int   mCaps4;
    unsigned int   mReserved2;
} DDSHeader;

GLuint LoadTextureS3TC( const char* TextureFileName )
{
    // Load the texture file
    char* pData = NULL;
    unsigned int fileSize = 0;
    
    ReadFile( TextureFileName, &pData, &fileSize );
    
    // Read the header
    DDSHeader* pHeader = (DDSHeader*)pData;

    // Generate handle
    GLuint handle;
    glGenTextures( 1, &handle );
    
    // Bind the texture
    glBindTexture( GL_TEXTURE_2D, handle );
    
    // Set filtering mode for 2D textures (bilinear filtering)
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    if( pHeader->mMipMapCount > 1 )
    {
        // Use mipmaps with bilinear filtering
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
    }
    
    // Determine texture format
    GLenum format;
    GLuint blockSize;
    switch( pHeader->mPixelFormat.mFourCC )
    {
        case 0x31545844: 
        {
            //FOURCC_DXT1
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            blockSize = 8;
            break;
        }
        case 0x33545844: 
        {
            //FOURCC_DXT3
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            blockSize = 16;
            break;
        }
        case 0x35545844: 
        {
            //FOURCC_DXT5
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            blockSize = 16;
            break;
        }
        default: 
        {
            // Unknown format
            assert(0);
            return 0;
        }
    }
   
    // Initialize the texture
    unsigned int offset = 0;
    unsigned int mipWidth = pHeader->mWidth;
    unsigned int mipHeight = pHeader->mHeight;

    unsigned int mip = 0;
    do
    {
        // Determine size
        // As defined in extension: size = ceil(<w>/4) * ceil(<h>/4) * blockSize
        unsigned int pixelDataSize = ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2) * blockSize;
    
        // Upload texture data for this mip
        glCompressedTexImage2D( GL_TEXTURE_2D, mip, format, mipWidth, mipHeight, 0, pixelDataSize, (pData + sizeof(DDSHeader)) + offset ); 
        CheckGlError( "glCompressedTexImage2D" );
        
        // Next mips is half the size (divide by 2) with a min of 1
        mipWidth = mipWidth >> 1;
        mipWidth = ( mipWidth == 0 ) ? 1 : mipWidth;

        mipHeight = mipHeight >> 1;
        mipHeight = ( mipHeight == 0 ) ? 1 : mipHeight; 

        // Move to next mip map
        offset += pixelDataSize;
        mip++;
    } while(mip < pHeader->mMipMapCount);

    // clean up
    free( pData );
        
    // Return handle
    return handle;
}

