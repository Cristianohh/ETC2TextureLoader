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

package com.intel.textureloader;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.View;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

///////////////////////////////////////////////////////////////////////////////////////////////////
// TextureLoaderView - main view that extends GLSurfaceView
class TextureLoaderView extends GLSurfaceView
{
    public TextureLoaderView( Context context ) 
    {
        super(context);

        // Create an OpenGL ES 3.0 context
        setEGLContextClientVersion( 3 );

        // Set the renderer associated with this view
        setRenderer( new Renderer() );
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Renderer is run on a separate thread and drives rendering
    private static class Renderer implements GLSurfaceView.Renderer 
    {
        public void onDrawFrame(GL10 gl) 
        {
            // Call our native function to render a frame
            TextureLoaderLib.drawFrame();
        }

        public void onSurfaceChanged( GL10 gl, int width, int height ) 
        {
            // Call our native function to set up graphics
            TextureLoaderLib.initGraphics( width, height );
        }

        public void onSurfaceCreated( GL10 gl, EGLConfig config ) 
        {
        }
    }
}
