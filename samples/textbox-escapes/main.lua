----------------------------------------------------------------
-- Copyright (c) 2010-2017 Zipline Games, Inc. 
-- All Rights Reserved. 
-- http://getmoai.com
----------------------------------------------------------------

MOAISim.openWindow ( "test", 512, 512 )
MOAIDebugLines.setStyle ( MOAIDebugLines.TEXT_BOX, 1, 1, 1, 1, 1 )
MOAIDebugLines.setStyle ( MOAIDebugLines.TEXT_BOX_BASELINES, 1, 1, 0, 0, 1 )
MOAIDebugLines.setStyle ( MOAIDebugLines.TEXT_BOX_LINES_LAYOUT_BOUNDS, 1, 0, 0, 1, 1 )

viewport = MOAIViewport.new ()
viewport:setSize ( 512, 512 )
viewport:setScale ( 512, -512 )

layer = MOAIPartitionViewLayer.new ()
layer:setViewport ( viewport )
layer:pushRenderPass ()

text = 'The quick <<foo>brown fox <<<jumps over the lazy dog.'

font = MOAIFont.new ()
font:load ( 'arial-rounded.TTF' )

textbox = MOAITextBox.new ()
textbox:setFont ( font )
textbox:setTextSize ( 24 )
textbox:setString ( text )
textbox:spool ()
textbox:setRect ( -128, -128, 128, 128 )
textbox:setAlignment ( MOAITextBox.CENTER_JUSTIFY, MOAITextBox.CENTER_JUSTIFY )
layer:insertProp ( textbox )

