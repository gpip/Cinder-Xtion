/*
* 
* Copyright (c) 2012, Ban the Rewind
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or 
* without modification, are permitted provided that the following 
* conditions are met:
* 
* Redistributions of source code must retain the above copyright 
* notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright 
* notice, this list of conditions and the following disclaimer in 
* the documentation and/or other materials provided with the 
* distribution.
* 
* Neither the name of the Ban the Rewind nor the names of its 
* contributors may be used to endorse or promote products 
* derived from this software without specific prior written 
* permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*/

// Includes
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Arcball.h"
#include "cinder/Sphere.h"
#include "cinder/Camera.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "CinderXtion.h"

/* 
* This application demonstrates how to represent the 
* Xtion's depth image in 3D space.
*/
class BasicApp : public ci::app::App
{

public:

	// Cinder callbacks
	void draw();
	void keyDown( ci::app::KeyEvent event );
	void mouseDown( ci::app::MouseEvent event );
	void mouseDrag( ci::app::MouseEvent event );
	void prepareSettings( Settings * settings );
	void shutdown();
	void setup();
	void update();

private:

	// Xtion
	Xtion::DeviceRef		mDevice;
	ci::ivec2				mInputSize;

	// Depth points
	std::vector<ci::vec3>	mPoints;

	// Camera
	ci::Arcball				mArcball;
	ci::Sphere			    mEarthSphere;
	ci::CameraPersp			mCamera;

	ci::Channel16u  		mDepth;
	ci::Surface8u   		mVideo;
	ci::Channel16u  		mUserImage;

	// Save screen shot
	void					screenShot();

};

// Imports
using namespace ci;
using namespace ci::app;
using namespace Xtion;
using namespace std;

// Render
void BasicApp::draw()
{

	// Clear window
	gl::viewport( 0, 0, getWindowWidth(), getWindowHeight() );
	gl::clear( Colorf::black() );

	// Half window size
	float width = getWindowCenter().x;
	float height = getWindowCenter().y;

	gl::setMatricesWindow( getWindowSize() );
	gl::color( ColorAf::white() );
	if ( mDepth.getWidth() ) {
		gl::draw( gl::Texture::create( mDepth ), mDepth.getBounds(), Rectf( 0.0f, height * 0.5f, width, height * 1.5f ) );
	}
	if ( mUserImage.getWidth() ) {
		gl::draw( gl::Texture::create( mUserImage ), mUserImage.getBounds(), Rectf( 0.0f, height * 0.5f, width, height * 1.5f ) );
	}
	if ( mVideo.getWidth() ) {
		gl::draw( gl::Texture::create( mVideo ), mVideo.getBounds(), Rectf( width, height * 0.5f, width * 2.0f, height * 1.5f ) );
	}
	gl::setMatrices( mCamera );
	gl::rotate( mArcball.getQuat() );

	// Draw point cloud
	gl::begin( GL_POINTS );
	for ( vector<vec3>::const_iterator pointIt = mPoints.cbegin(); pointIt != mPoints.cend(); ++pointIt ) {
		float depth = 1.0f - pointIt->z / ( mCamera.getEyePoint().z * -2.0f );
		gl::color( ColorAf( 1.0f, depth, 1.0f - depth, depth ) );
		gl::vertex( *pointIt );
	}
	gl::end();

}

// Handles key press
void BasicApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() ) {
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_s:
		screenShot();
		break;
	}
}

void BasicApp::mouseDown( ci::app::MouseEvent event )
{
	mArcball.mouseDown( event );
}

void BasicApp::mouseDrag( ci::app::MouseEvent event )
{
	mArcball.mouseDrag( event );
}

// Prepare window
void BasicApp::prepareSettings( Settings * settings )
{
	settings->setFrameRate( 60.0f );
	settings->setWindowSize( 800, 600 );
}

// Take screen shot
void BasicApp::screenShot()
{
	writeImage( getAppPath() / fs::path( "frame" + toString( getElapsedFrames() ) + ".png" ), copyWindowSurface() );
}

// Set up
void BasicApp::setup()
{

	// Set up OpenGL
	gl::enable( GL_DEPTH_TEST );
	//glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
	//glEnable( GL_POINT_SMOOTH );
	glPointSize( 0.25f );
	gl::enableAlphaBlending();
	gl::enableAdditiveBlending();
	gl::color( ColorAf::white() );

	mInputSize = ivec2();

	// Start Xtion
	mDevice = Device::create();
	mDevice->start( getAssetPath("config.xml") );

	// Set up camera
	mCamera.lookAt( vec3( 0.0f, 0.0f, 670.0f ), vec3() );
	mCamera.setPerspective( 60.0f, getWindowAspectRatio(), 0.01f, 5000.0f );
	mEarthSphere = Sphere( vec3( 0 ), 1 );
	mArcball = Arcball( &mCamera, mEarthSphere );

}

// Called on exit
void BasicApp::shutdown()
{
	mDevice->stop();
	mPoints.clear();
}

// Runs update logic
void BasicApp::update()
{

	// Device is capturing
	if ( mDevice->isCapturing() ) {
		if ( mDevice->checkNewDepthFrame() ) {
			mDepth = mDevice->getDepth();
		}
		if ( mDevice->checkNewUserData() ) {
			mUserImage = mDevice->getUserImage();
		}
		if ( mDevice->checkNewVideoFrame() ) {
			mVideo = mDevice->getVideo();
		}
	}

}

// Run application
CINDER_APP( BasicApp, RendererGl )
