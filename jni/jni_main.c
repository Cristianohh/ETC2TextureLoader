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
#include <jni.h>
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "file.h"
#include "texture.h"

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
// Simple geometry data to render textured quads
typedef struct
{
    float x;
    float y;
    float u;
    float v;
    
} TriangleVertex;

TriangleVertex gTriangleVerticesPNG[] =     { { -0.6f,  1.0f, 0.2f, 0.0f  },
                                            {   -1.0f, -1.0f, 0.0f,  1.0f  },
                                            {   -0.6f, -1.0f, 0.2f, 1.0f  },
                                            {   -1.0f,  1.0f, 0.0f,  0.0f  },
                                            {   -1.0f, -1.0f, 0.0f,  1.0f  },
                                            {   -0.6f,  1.0f, 0.2f, 0.0f  } }; 
                                           
TriangleVertex gTriangleVerticesETC[] =     { { -0.2f,  1.0f, 0.4f,  0.0f  },
                                            {   -0.6f, -1.0f, 0.2f, 1.0f  },
                                            {   -0.2f, -1.0f, 0.4f,  1.0f  },
                                            {   -0.6f,  1.0f, 0.2f, 0.0f  },
                                            {   -0.6f, -1.0f, 0.2f, 1.0f  },
                                            {   -0.2f,  1.0f, 0.4f,  0.0f  } };

TriangleVertex gTriangleVerticesETC2[] =     { { 0.2f,  1.0f, 0.6f,  0.0f  },
                                            {   -0.2f, -1.0f, 0.4f, 1.0f  },
                                            {    0.2f, -1.0f, 0.6f,  1.0f  },
                                            {   -0.2f,  1.0f, 0.4f, 0.0f  },
                                            {   -0.2f, -1.0f, 0.4f, 1.0f  },
                                            {    0.2f,  1.0f, 0.6f,  0.0f  } };

TriangleVertex gTriangleVerticesPVRTC[] =   { {  0.6f,  1.0f, 0.8f, 0.0f  },
                                            {    0.2f, -1.0f, 0.6f,  1.0f  },
                                            {    0.6f, -1.0f, 0.8f, 1.0f  },
                                            {    0.2f,  1.0f, 0.6f,  0.0f  },
                                            {    0.2f, -1.0f, 0.6f,  1.0f  },
                                            {    0.6f,  1.0f, 0.8f, 0.0f  } }; 
                                           
TriangleVertex gTriangleVerticesS3TC[] =    { {  1.0f,  1.0f, 1.0f,  0.0f  },
                                            {    0.6f, -1.0f, 0.8f, 1.0f  },
                                            {    1.0f, -1.0f, 1.0f,  1.0f  },
                                            {    0.6f,  1.0f, 0.8f, 0.0f  },
                                            {    0.6f, -1.0f, 0.8f, 1.0f  },
                                            {    1.0f,  1.0f, 1.0f,  0.0f  } }; 


///////////////////////////////////////////////////////////////////////////////////////////////////
// Simple vertex shader
static const char gVertexShader[] = 
    "attribute vec4 aPosition;  \n"
    "attribute vec2 aTexCoord;  \n"
    "varying vec2 vTexCoord;    \n"
    "void main()                \n"
    "{                          \n"
    "  vTexCoord = aTexCoord;   \n"
    "  gl_Position = aPosition; \n"
    "}                          \n";


///////////////////////////////////////////////////////////////////////////////////////////////////
// Simple pixel (aka Fragment) shader
static const char gPixelShader[] = 
    "precision mediump float;                          \n"
    "varying vec2 vTexCoord;                           \n"
    "uniform sampler2D sTexture;                       \n"
    "void main()                                       \n"
    "{                                                 \n"
    "  gl_FragColor = texture2D(sTexture, vTexCoord);  \n"
    "}                                                 \n";


///////////////////////////////////////////////////////////////////////////////////////////////////
// CompileShader - Compiles the passed in string for the given shaderType
GLuint CompileShader( GLenum shaderType, const char* pSource ) 
{
    // Create a shader handle
    GLuint shaderHandle = glCreateShader( shaderType );

    if( shaderHandle ) 
    {
        // Set the shader source
        glShaderSource( shaderHandle, 1, &pSource, NULL );

        // Compile the shader
        glCompileShader( shaderHandle );

        // Check the compile status
        GLint compileStatus = 0;
        glGetShaderiv( shaderHandle, GL_COMPILE_STATUS, &compileStatus );

        if( !compileStatus ) 
        {
            // There was an error, print it out
            GLint infoLogLength = 0;
            glGetShaderiv( shaderHandle, GL_INFO_LOG_LENGTH, &infoLogLength );

            if( infoLogLength > 1 ) 
            {
                char* pShaderInfoLog = (char*)malloc( infoLogLength );
                if( pShaderInfoLog ) 
                {
                    glGetShaderInfoLog( shaderHandle, infoLogLength, NULL, pShaderInfoLog );
                    LogError( "Error compiling shader: \n%s", pShaderInfoLog );
                    free( pShaderInfoLog );
                }

                // Free the handle
                glDeleteShader( shaderHandle );
                shaderHandle = 0;
            }
        }
    }

    return shaderHandle;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// createProgram - Creates a new program with the given vertex and pixel shader
GLuint createProgram( const char* pVertexSource, const char* pPixelSource ) 
{
    // Compile the vertex shader
    GLuint vertexShaderHandle = CompileShader( GL_VERTEX_SHADER, pVertexSource );
    GLuint pixelShaderHandle  = CompileShader( GL_FRAGMENT_SHADER, pPixelSource );

    if( !vertexShaderHandle || !pixelShaderHandle )
    {
        return 0;
    }

    // Create a new handle for this program
    GLuint programHandle = glCreateProgram();

    // Link and finish the program
    if( programHandle ) 
    {
        // Set the vertex shader for this program
        glAttachShader( programHandle, vertexShaderHandle );
        CheckGlError( "glAttachShader" );

        // Set the pixel shader for this program
        glAttachShader( programHandle, pixelShaderHandle );
        CheckGlError( "glAttachShader" );

        // Link the program
        glLinkProgram( programHandle );

        // Check the link status
        GLint linkStatus = 0;
        glGetProgramiv( programHandle, GL_LINK_STATUS, &linkStatus );

        if( !linkStatus ) 
        {
            // There was an error, print it out
            GLint infoLogLength = 0;
            glGetProgramiv( programHandle, GL_INFO_LOG_LENGTH, &infoLogLength );

            if( infoLogLength ) 
            {
                char* pInfoLog = (char*)malloc( infoLogLength );
                if( pInfoLog ) 
                {
                    glGetProgramInfoLog( programHandle, infoLogLength, NULL, pInfoLog );
                    LogError( "Error linking the program: \n%s", pInfoLog );
                    free( pInfoLog );
                }
            }

            // Free the handle
            glDeleteProgram( programHandle );
            programHandle = 0;
        }
    }

    return programHandle;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Init - Called from Java-side to setup the graphics on the native-side 
GLuint gProgramHandle;
GLuint gaPositionHandle;
GLuint gaTexCoordHandle;
GLuint gaTexSamplerHandle;

GLuint gTextureHandlePNG;
GLuint gTextureHandleUnsupported;
GLuint gTextureHandleETC;
GLuint gTextureHandleETC2;
GLuint gTextureHandlePVRTC;
GLuint gTextureHandleS3TC;

void Init( int width, int height ) 
{
    // Init the shaders
    gProgramHandle = createProgram( gVertexShader, gPixelShader );
    if( !gProgramHandle ) 
    {
        LogError( "Could not create program." );
        assert(0);
    }
    
    // Read attribute locations from the program
    gaPositionHandle = glGetAttribLocation( gProgramHandle, "aPosition" );
    CheckGlError( "glGetAttribLocation" );

    gaTexCoordHandle = glGetAttribLocation( gProgramHandle, "aTexCoord" );
    CheckGlError( "glGetAttribLocation" );
    
    gaTexSamplerHandle = glGetUniformLocation( gProgramHandle, "sTexture" );
    CheckGlError( "glGetUnitformLocation" );

    // Set up viewport
    glViewport( 0, 0, width, height ) ;
    CheckGlError( "glViewport" );

    // Load textures
    gTextureHandlePNG = LoadTexturePNG("tex_png.png");
    gTextureHandleUnsupported = LoadTexturePNG("tex_bw.png");
    gTextureHandleETC = ( IsETCSupported() ) ? LoadTextureETC_KTX("tex_etc1.ktx") : gTextureHandleUnsupported;
    gTextureHandleETC2 = ( IsETC2Supported() ) ? LoadTextureETC_KTX("tex_etc2.ktx") : gTextureHandleUnsupported;
    gTextureHandlePVRTC = ( IsPVRTCSupported() ) ? LoadTexturePVRTC("tex_pvr.pvr") : gTextureHandleUnsupported;
    gTextureHandleS3TC = ( IsS3TCSupported() ) ? LoadTextureS3TC("tex_s3tc.dds") : gTextureHandleUnsupported;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Render - Called from Java-side to render a frame
void Render() 
{
    // Set the clear color
    glClearColor( 0.8f, 0.7f, 0.6f, 1.0f);
    CheckGlError( "glClearColor" );

    // Clear the color and depth buffer 
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    CheckGlError( "glClear" );
    
    // Select vertex/pixel shader
    glUseProgram( gProgramHandle );
    CheckGlError( "glUseProgram" );
    
    // Enable vertex
    glEnableVertexAttribArray( gaPositionHandle );
    CheckGlError( "glEnableVertexAttribArray" );
    
    // Enable tex coords
    glEnableVertexAttribArray( gaTexCoordHandle );
    CheckGlError( "glEnableVertexAttribArray" );
 
    // Set texture sampler
    glActiveTexture( GL_TEXTURE0 );
    
    // Enable texture sampler 
    glUniform1i( gaTexSamplerHandle, 0 );
    
    // PNG //////////////////////////////////////////////////////////////////////////////////////////////////////
            
    // Set vertex position
    glVertexAttribPointer( gaPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), gTriangleVerticesPNG );
    CheckGlError( "glVertexAttribPointer" );
    
    // Set vertex texture coordinates
    glVertexAttribPointer( gaTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (char*)gTriangleVerticesPNG + sizeof(float)*2 );
    CheckGlError( "glVertexAttribPointer" );
    
    // Set active texture
    glBindTexture( GL_TEXTURE_2D, gTextureHandlePNG );
    
    glDrawArrays( GL_TRIANGLES, 0, 6 );

    // ETC //////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Set vertex position
    glVertexAttribPointer( gaPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), gTriangleVerticesETC );
    CheckGlError( "glVertexAttribPointer");
    
    // Set vertex texture coordinates
    glVertexAttribPointer( gaTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (char*)gTriangleVerticesETC + sizeof(float)*2 );
    CheckGlError( "glVertexAttribPointer");
    
    // Set active texture
    glBindTexture( GL_TEXTURE_2D, gTextureHandleETC );

    glDrawArrays( GL_TRIANGLES, 0, 6 );

    // ETC2 //////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Set vertex position
    glVertexAttribPointer( gaPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), gTriangleVerticesETC2 );
    CheckGlError( "glVertexAttribPointer");
    
    // Set vertex texture coordinates
    glVertexAttribPointer( gaTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (char*)gTriangleVerticesETC2 + sizeof(float)*2 );
    CheckGlError( "glVertexAttribPointer");
    
    // Set active texture
    glBindTexture( GL_TEXTURE_2D, gTextureHandleETC2 );

    glDrawArrays( GL_TRIANGLES, 0, 6 );

    // PVRTC //////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Set vertex position
    glVertexAttribPointer( gaPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), gTriangleVerticesPVRTC );
    CheckGlError( "glVertexAttribPointer" );
    
    // Set vertex texture coordinates
    glVertexAttribPointer( gaTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (char*)gTriangleVerticesPVRTC + sizeof(float)*2 );
    CheckGlError( "glVertexAttribPointer" );
    
    // Set active texture
    glBindTexture( GL_TEXTURE_2D, gTextureHandlePVRTC );
    
    glDrawArrays( GL_TRIANGLES, 0, 6) ;

    // S3TC //////////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Set vertex position
    glVertexAttribPointer( gaPositionHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), gTriangleVerticesS3TC );
    CheckGlError( "glVertexAttribPointer" );
    
    // Set vertex texture coordinates
    glVertexAttribPointer( gaTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (char*)gTriangleVerticesS3TC + sizeof(float)*2 );
    CheckGlError( "glVertexAttribPointer" );
    
    // Set active texture
    glBindTexture( GL_TEXTURE_2D, gTextureHandleS3TC );

    glDrawArrays( GL_TRIANGLES, 0, 6 );
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// JNI helper functions
JNIEXPORT void JNICALL Java_com_intel_textureloader_TextureLoaderLib_initGraphics( JNIEnv * env, jobject obj,  jint width, jint height )
{
    Init( width, height );
}

JNIEXPORT void JNICALL Java_com_intel_textureloader_TextureLoaderLib_drawFrame( JNIEnv * env, jobject obj )
{
    Render();
}

JNIEXPORT void JNICALL Java_com_intel_textureloader_TextureLoaderLib_createAssetManager(JNIEnv* env, jobject obj, jobject assetManager)
{
    AAssetManager* mgr = AAssetManager_fromJava( env, assetManager );
    
    // Store the assest manager for future use.
    SetAssetManager( mgr );   
}


