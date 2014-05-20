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

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;

public class TextureLoaderActivity extends Activity 
{
    static AssetManager sAssetManager;
    TextureLoaderView mView;

    // On applications creation
    @Override protected void onCreate( Bundle savedInstanceState ) 
    {
        super.onCreate( savedInstanceState );
        
        // Pass the asset manager to the native code
        sAssetManager = getAssets();
        TextureLoaderLib.createAssetManager( sAssetManager );
        
        // Create our view for OpenGL rendering
        mView = new TextureLoaderView( getApplication() );
        
        setContentView( mView );
    }

    @Override protected void onPause() 
    {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() 
    {
        super.onResume();
        mView.onResume();
    }
}
