// Copyright (c) 2010-2017 Zipline Games, Inc. All Rights Reserved.
// http://getmoai.com

#include "pch.h"
#include <moai-sim/MOAIGfxMgr.h>
#include <moai-sim/MOAIGfxResourceClerk.h>
#include <moai-sim/MOAIImage.h>
#include <moai-sim/MOAISingleTexture.h>

//================================================================//
// local
//================================================================//

//----------------------------------------------------------------//
/**	@lua	getSize
	@text	Returns the width and height of the texture's source image.
			Avoid using the texture width and height to compute UV
			coordinates from pixels, as this will prevent texture
			resolution swapping.
	
	@in		MOAISingleTexture self
	@out	number width
	@out	number height
*/
int MOAISingleTexture::_getSize ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAISingleTexture, "U" )
	
	self->DoCPUCreate ();
	
	lua_pushnumber ( state, self->mWidth );
	lua_pushnumber ( state, self->mHeight );
	
	return 2;
}

//----------------------------------------------------------------//
/**	@lua	release
	@text	Releases any memory associated with the texture.
	
	@in		MOAISingleTexture self
	@out	nil
*/
int MOAISingleTexture::_release ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAISingleTexture, "U" )
	
	self->Destroy ();
	
	return 0;
}

//----------------------------------------------------------------//
/**	@lua	setDebugName
	@text	Set a name for the texture to use during memory logging.
	
	@in		MOAISingleTexture self
	@in		string debugName
	@out	nil
*/
int MOAISingleTexture::_setDebugName ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAISingleTexture, "U" )

	self->mDebugName = state.GetValue < cc8* >( 2, "" );
	return 0;
}

//----------------------------------------------------------------//
/**	@lua	setFilter
	@text	Set default filtering mode for texture.
	
	@in		MOAISingleTexture self
	@in		number min			One of MOAISingleTexture.GL_LINEAR, MOAISingleTexture.GL_LINEAR_MIPMAP_LINEAR, MOAISingleTexture.GL_LINEAR_MIPMAP_NEAREST,
								MOAISingleTexture.GL_NEAREST, MOAISingleTexture.GL_NEAREST_MIPMAP_LINEAR, MOAISingleTexture.GL_NEAREST_MIPMAP_NEAREST
	@opt	number mag			Defaults to value passed to 'min'.
	@out	nil
*/
int MOAISingleTexture::_setFilter ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAISingleTexture, "UN" )

	int min = state.GetValue < int >( 2, ZGL_SAMPLE_LINEAR );
	int mag = state.GetValue < int >( 3, min );

	MOAISingleTexture::CheckFilterModes ( min, mag );

	self->SetFilter ( min, mag );

	return 0;
}

//----------------------------------------------------------------//
/**	@lua	setWrap
	@text	Set wrapping mode for texture.
	
	@in		MOAISingleTexture self
	@in		boolean wrap		Texture will wrap if true, clamp if not.
	@out	nil
*/
int MOAISingleTexture::_setWrap ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAISingleTexture, "UB" )
	
	bool wrap = state.GetValue < bool >( 2, false );
	
	self->mWrap = wrap ? ZGL_WRAP_MODE_REPEAT : ZGL_WRAP_MODE_CLAMP;

	return 0;
}

//================================================================//
// MOAISingleTexture
//================================================================//

//----------------------------------------------------------------//
void MOAISingleTexture::CheckFilterModes ( int min, int mag ) {
	UNUSED ( min );
	UNUSED ( mag );

	#ifdef _DEBUG
		switch ( min ) {
			case ZGL_SAMPLE_LINEAR_MIPMAP_LINEAR:
			case ZGL_SAMPLE_LINEAR_MIPMAP_NEAREST:
			case ZGL_SAMPLE_NEAREST_MIPMAP_LINEAR:
			case ZGL_SAMPLE_NEAREST_MIPMAP_NEAREST:
		
				if ( !(( mag == ZGL_SAMPLE_LINEAR ) || ( mag == ZGL_SAMPLE_NEAREST ))) {

					ZLLog_Warning ( "WARNING: possibly incompatible filter modes; MIPMAP not supported for mag filter\n" );
				}
				break;
				
			default:
				break;
		}
	#endif
}

//----------------------------------------------------------------//
void MOAISingleTexture::CleanupOnError () {

	this->mTextureSize = 0;
	MOAIGfxMgr::GetDrawingAPI ().Delete ( this->mGLTexID );
	this->mGLTexID = 0;
	this->mWidth = 0;
	this->mHeight = 0;
	this->mTextureSize = 0;
}

//----------------------------------------------------------------//
u32 MOAISingleTexture::CountActiveUnits () {

	return 1;
}

//----------------------------------------------------------------//
bool MOAISingleTexture::CreateTextureFromImage ( MOAIImage& srcImage ) {

	if ( !MOAIGfxMgr::Get ().GetHasContext ()) return false;

	ZLGfx& gfx = MOAIGfxMgr::GetDrawingAPI ();

	MOAIImage altImage;

	ZLColor::ColorFormat colorFormat = srcImage.GetColorFormat ();
	if ( colorFormat == ZLColor::CLR_FMT_UNKNOWN ) return false;

	if (( colorFormat == ZLColor::A_1 ) || ( colorFormat == ZLColor::A_4 )) {
		colorFormat = ZLColor::A_8;
	}
	
	if (( colorFormat != srcImage.GetColorFormat ()) || ( srcImage.GetPixelFormat () != MOAIImage::TRUECOLOR )) {
		//if ( !altImage.Convert ( srcImage, colorFormat, MOAIImage::TRUECOLOR )); // TODO: what was this? add error handling?
		altImage.Convert ( srcImage, colorFormat, MOAIImage::TRUECOLOR );
	}
	
	MOAIImage& image = altImage.IsOK () ? altImage : srcImage;
	if ( !image.IsOK ()) return false;

	//MOAIGfxMgr::Get ().ClearErrors ();
	this->mGLTexID = gfx.CreateTexture ();
	if ( !this->mGLTexID ) return false;

	// get the dimensions before trying to get the OpenGL texture ID
	this->mWidth = image.GetWidth ();
	this->mHeight = image.GetHeight ();

	// warn if not a power of two (if we're supposed to generate mipmaps)
	if ( this->ShouldGenerateMipmaps () && !image.IsPow2 ()) {
		MOAILogF ( 0, ZLLog::LOG_WARNING, MOAILogMessages::MOAITexture_NonPowerOfTwo_SDD, ( cc8* )this->mDebugName, this->mWidth, this->mHeight );
	}

	// GL_ALPHA
	// GL_RGB
	// GL_RGBA
	// GL_LUMINANCE
	// GL_LUMINANCE_ALPHA

	// GL_UNSIGNED_BYTE
	// GL_UNSIGNED_SHORT_5_6_5
	// GL_UNSIGNED_SHORT_4_4_4_4
	// GL_UNSIGNED_SHORT_5_5_5_1

	switch ( colorFormat ) {
		
		case ZLColor::A_8:
			this->mGLInternalFormat = ZGL_PIXEL_FORMAT_ALPHA;
			this->mGLPixelType = ZGL_PIXEL_TYPE_UNSIGNED_BYTE;
			break;
		
		case ZLColor::LA_8:
			this->mGLInternalFormat = ZGL_PIXEL_FORMAT_LUMINANCE_ALPHA;
			this->mGLPixelType = ZGL_PIXEL_TYPE_UNSIGNED_BYTE;
			break;
		
		case ZLColor::RGB_888:
			this->mGLInternalFormat = ZGL_PIXEL_FORMAT_RGB;
			this->mGLPixelType = ZGL_PIXEL_TYPE_UNSIGNED_BYTE;
			break;
		
		case ZLColor::RGB_565:
			this->mGLInternalFormat = ZGL_PIXEL_FORMAT_RGB;
			this->mGLPixelType = ZGL_PIXEL_TYPE_UNSIGNED_SHORT_5_6_5;
			break;
		
		case ZLColor::RGBA_5551:
			this->mGLInternalFormat = ZGL_PIXEL_FORMAT_RGBA;
			this->mGLPixelType = ZGL_PIXEL_TYPE_UNSIGNED_SHORT_5_5_5_1;
			break;
		
		case ZLColor::RGBA_4444:
			this->mGLInternalFormat = ZGL_PIXEL_FORMAT_RGBA;
			this->mGLPixelType = ZGL_PIXEL_TYPE_UNSIGNED_SHORT_4_4_4_4;
			break;
		
		case ZLColor::RGBA_8888:
			this->mGLInternalFormat = ZGL_PIXEL_FORMAT_RGBA;
			this->mGLPixelType = ZGL_PIXEL_TYPE_UNSIGNED_BYTE;
			break;
			
		default:
			this->CleanupOnError ();
			return false;
	}

	gfx.SetTexture ( this->mGLTexID );

	gfx.TexImage2D (
		0,
		this->mGLInternalFormat,
		this->mWidth,  
		this->mHeight,  
		this->mGLInternalFormat,
		this->mGLPixelType,
		image.GetBitmapBuffer ()
	);
	
	this->mTextureSize = image.GetBitmapSize ();

	// TODO: error handling
//	if ( MOAIGfxMgr::Get ().LogErrors ()) {
//		this->CleanupOnError ();
//		return false;
//	}
//	else if ( this->ShouldGenerateMipmaps ()) {
	
	if ( this->ShouldGenerateMipmaps ()) {
	
		u32 mipLevel = 1;
		
		MOAIImage mipmap;
		mipmap.Copy ( image );
		
		while ( mipmap.MipReduce ()) {
			
			gfx.TexImage2D (
				mipLevel++,  
				this->mGLInternalFormat,
				mipmap.GetWidth (),  
				mipmap.GetHeight (),  
				this->mGLInternalFormat,
				this->mGLPixelType,
				mipmap.GetBitmapBuffer ()
			);
			
			if ( MOAIGfxMgr::Get ().LogErrors ()) {
				this->CleanupOnError ();
				return false;
			}
			this->mTextureSize += mipmap.GetBitmapSize ();
		}
	}
	
	//MOAIGfxMgr::Get ().ReportTextureAlloc ( this->mDebugName, this->mTextureSize );
	
	return true;
}

//----------------------------------------------------------------//
u32 MOAISingleTexture::GetHeight () {
	return this->mHeight;
}

//----------------------------------------------------------------//
MOAISingleTexture* MOAISingleTexture::GetTextureForUnit ( u32 unit ) {
	UNUSED ( unit );

	assert ( unit == 0 );

	return this;
}

//----------------------------------------------------------------//
u32 MOAISingleTexture::GetWidth () {
	return this->mWidth;
}

//----------------------------------------------------------------//
MOAISingleTexture::MOAISingleTexture () :
	mGLTexID ( 0 ),
	mWidth ( 0 ),
	mHeight ( 0 ),
	mMinFilter ( ZGL_SAMPLE_LINEAR ),
	mMagFilter ( ZGL_SAMPLE_NEAREST ),
	mWrap ( ZGL_WRAP_MODE_CLAMP ),
	mTextureSize ( 0 ) {
	
	RTTI_BEGIN
		RTTI_EXTEND ( MOAILuaObject )
		RTTI_EXTEND ( MOAIGfxResource )
	RTTI_END
}

//----------------------------------------------------------------//
MOAISingleTexture::~MOAISingleTexture () {

	this->OnGPUDeleteOrDiscard ( true );
}

//----------------------------------------------------------------//
bool MOAISingleTexture::OnCPUCreate () {

	return true;
}

//----------------------------------------------------------------//
void MOAISingleTexture::OnCPUDestroy () {
}

//----------------------------------------------------------------//
void MOAISingleTexture::OnGfxEvent ( u32 event, void* userdata ) {

	MOAIGfxMgr::Get ().ReportTextureAlloc ( this->mDebugName, this->mTextureSize );
	MOAIGfxResource::OnGfxEvent ( event, userdata );
}

//----------------------------------------------------------------//
void MOAISingleTexture::OnGPUBind () {

	MOAIGfxMgr::GetDrawingAPI ().SetTexture ( this->mGLTexID );
}

//----------------------------------------------------------------//
void MOAISingleTexture::OnGPUDeleteOrDiscard ( bool shouldDelete ) {

	if ( this->mGLTexID && MOAIGfxMgr::IsValid ()) {
		MOAIGfxMgr::Get ().ReportTextureFree ( this->mDebugName, this->mTextureSize );
	}
	MOAIGfxResourceClerk::DeleteOrDiscardHandle ( this->mGLTexID, shouldDelete );
}

//----------------------------------------------------------------//
void MOAISingleTexture::OnGPUUnbind () {

	MOAIGfxMgr::GetDrawingAPI ().SetTexture ( 0 );
}

//----------------------------------------------------------------//
bool MOAISingleTexture::OnGPUUpdate () {

	ZLGfx& gfx = MOAIGfxMgr::GetDrawingAPI ();

	gfx.TexParameteri ( ZGL_TEXTURE_WRAP_S, this->mWrap );
	gfx.TexParameteri ( ZGL_TEXTURE_WRAP_T, this->mWrap );
		
	gfx.TexParameteri ( ZGL_TEXTURE_MIN_FILTER, this->mMinFilter );
	gfx.TexParameteri ( ZGL_TEXTURE_MAG_FILTER, this->mMagFilter );
	
	return true;
}

//----------------------------------------------------------------//
void MOAISingleTexture::RegisterLuaClass ( MOAILuaState& state ) {
	
	MOAIGfxResource::RegisterLuaClass ( state );
	
	state.SetField ( -1, "GL_LINEAR",					( u32 )ZGL_SAMPLE_LINEAR );
	state.SetField ( -1, "GL_LINEAR_MIPMAP_LINEAR",		( u32 )ZGL_SAMPLE_LINEAR_MIPMAP_LINEAR );
	state.SetField ( -1, "GL_LINEAR_MIPMAP_NEAREST",	( u32 )ZGL_SAMPLE_LINEAR_MIPMAP_NEAREST );
	
	state.SetField ( -1, "GL_NEAREST",					( u32 )ZGL_SAMPLE_NEAREST );
	state.SetField ( -1, "GL_NEAREST_MIPMAP_LINEAR",	( u32 )ZGL_SAMPLE_NEAREST_MIPMAP_LINEAR );
	state.SetField ( -1, "GL_NEAREST_MIPMAP_NEAREST",	( u32 )ZGL_SAMPLE_NEAREST_MIPMAP_NEAREST );
	
	state.SetField ( -1, "GL_RGBA4",					( u32 )ZGL_PIXEL_FORMAT_RGBA4 );
	state.SetField ( -1, "GL_RGB5_A1",					( u32 )ZGL_PIXEL_FORMAT_RGB5_A1 );
	state.SetField ( -1, "GL_DEPTH_COMPONENT16",		( u32 )ZGL_PIXEL_FORMAT_DEPTH_COMPONENT16 );
	//***state.SetField ( -1, "GL_DEPTH_COMPONENT24",	( u32 )GL_DEPTH_COMPONENT24 );
	//***state.SetField ( -1, "GL_STENCIL_INDEX1",		( u32 )GL_STENCIL_INDEX1 );
	//***state.SetField ( -1, "GL_STENCIL_INDEX4",		( u32 )GL_STENCIL_INDEX4 );
	state.SetField ( -1, "GL_STENCIL_INDEX8",			( u32 )ZGL_PIXEL_FORMAT_STENCIL_INDEX8 );
	//***state.SetField ( -1, "GL_STENCIL_INDEX16",		( u32 )GL_STENCIL_INDEX16 );
	
	// TODO:
	#ifdef MOAI_OS_ANDROID
		state.SetField ( -1, "GL_RGB565",				( u32 )ZGL_PIXEL_FORMAT_RGB565 );
	#else
		state.SetField ( -1, "GL_RGBA8",				( u32 )ZGL_PIXEL_FORMAT_RGBA8 );
	#endif
}

//----------------------------------------------------------------//
void MOAISingleTexture::RegisterLuaFuncs ( MOAILuaState& state ) {

	MOAIGfxResource::RegisterLuaFuncs ( state );

	luaL_Reg regTable [] = {
		{ "getSize",				_getSize },
		{ "release",				_release },
		{ "setDebugName",			_setDebugName },
		{ "setFilter",				_setFilter },
		{ "setWrap",				_setWrap },
		{ NULL, NULL }
	};

	luaL_register ( state, 0, regTable );
}

//----------------------------------------------------------------//
void MOAISingleTexture::SerializeIn ( MOAILuaState& state, MOAIDeserializer& serializer ) {
	UNUSED ( state );
	UNUSED ( serializer );
}

//----------------------------------------------------------------//
void MOAISingleTexture::SerializeOut ( MOAILuaState& state, MOAISerializer& serializer ) {
	UNUSED ( state );
	UNUSED ( serializer );
}

//----------------------------------------------------------------//
void MOAISingleTexture::SetFilter ( int filter ) {

	this->SetFilter ( filter, filter );
}

//----------------------------------------------------------------//
void MOAISingleTexture::SetFilter ( int min, int mag ) {

	this->mMinFilter = min;
	this->mMagFilter = mag;
	
	this->ScheduleForGPUUpdate ();
}

//----------------------------------------------------------------//
void MOAISingleTexture::SetTextureID ( ZLGfxHandle* glTexID, int internalFormat, int pixelType, size_t textureSize ) {

	this->mGLTexID = glTexID;
	this->mGLInternalFormat = internalFormat;
	this->mGLPixelType = pixelType;
	this->mTextureSize = textureSize;

	this->ScheduleForGPUUpdate ();
}

//----------------------------------------------------------------//
void MOAISingleTexture::SetWrap ( int wrap ) {

	this->mWrap = wrap;
	
	this->ScheduleForGPUUpdate ();
}

//----------------------------------------------------------------//
bool MOAISingleTexture::ShouldGenerateMipmaps () {

	return (
		( this->mMinFilter == ZGL_SAMPLE_LINEAR_MIPMAP_LINEAR ) ||
		( this->mMinFilter == ZGL_SAMPLE_LINEAR_MIPMAP_NEAREST ) ||
		( this->mMinFilter == ZGL_SAMPLE_NEAREST_MIPMAP_LINEAR ) ||
		( this->mMinFilter == ZGL_SAMPLE_NEAREST_MIPMAP_NEAREST )
	);
}

//----------------------------------------------------------------//
bool MOAISingleTexture::UpdateTextureFromImage ( MOAIImage& image, ZLIntRect rect ) {

	// TODO: what happens when image is an unsupported format?

	// if we need to generate mipmaps or the dimensions have changed, clear out the old texture
	if ( this->ShouldGenerateMipmaps () || ( this->mWidth != image.GetWidth ()) || ( this->mHeight != image.GetHeight ())) {
	
		MOAIGfxMgr::Get ().ReportTextureFree ( this->mDebugName, this->mTextureSize );
		MOAIGfxResourceClerk::DeleteOrDiscardHandle ( this->mGLTexID, false );
		
		if ( this->CreateTextureFromImage ( image )) {
			MOAIGfxMgr::Get ().ReportTextureAlloc ( this->mDebugName, this->mTextureSize );
			return true;
		}
		return false;
	}
	
	ZLGfx& gfx = MOAIGfxMgr::GetDrawingAPI ();
	
	// if the texture exists just update the sub-region
	// otherwise create a new texture from the image
	if ( this->mGLTexID ) {

		gfx.SetTexture ( this->mGLTexID );

		rect.Bless ();
		ZLIntRect imageRect = image.GetRect ();
		imageRect.Clip ( rect );
		
		ZLSharedConstBuffer* bitmapBuffer = image.GetBitmapBuffer ();
		
		MOAIImage subImage;
		if (( this->mWidth != ( u32 )rect.Width ()) || ( this->mHeight != ( u32 )rect.Height ())) {
			subImage.GetSubImage ( image, rect ); // TODO: need to convert to correct format for texture
			bitmapBuffer = subImage.GetBitmapBuffer ();
		}

		gfx.TexSubImage2D (
			0,
			rect.mXMin,
			rect.mYMin,
			rect.Width (),
			rect.Height (),
			this->mGLInternalFormat,
			this->mGLPixelType,  
			bitmapBuffer
		);
		
		MOAIGfxMgr::Get ().LogErrors ();
		
		return true;
	}
	
	return false;
}
