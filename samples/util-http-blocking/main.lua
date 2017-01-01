----------------------------------------------------------------
-- Copyright (c) 2010-2017 Zipline Games, Inc. 
-- All Rights Reserved. 
-- http://getmoai.com
----------------------------------------------------------------

function onFinish ( task, responseCode )

	print ( "onFinish" )
	print ( responseCode )

	if ( task:getSize ()) then
		print ( task:getString ())
	else
		print ( "nothing" )
	end
end

task = MOAIHttpTask.new ()

task:setVerb ( MOAIHttpTask.HTTP_GET )
task:setUrl ( "http://www.cnn.com" )
task:setCallback ( onFinish )
task:setUserAgent ( "Moai" )
task:setHeader ( "Foo", "foo" )
task:setHeader ( "Bar", "bar" )
task:setHeader ( "Baz", "baz" )
task:setVerbose ( true )
task:performSync ()
