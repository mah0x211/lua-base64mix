local base64 = require('base64mix');
local std = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
local url = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_';
local mix1 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-/';
local mix2 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+_';
local srcStd, srcUrl, srcMix, enc;

-- base64-std
srcStd = ifNil( base64.decode( std ) );
enc = ifNil( base64.encode( srcStd ) );
ifNotEqual( std, enc );
-- decode mix
srcStd = ifNil( base64.decodeMix( std ) );
enc = ifNil( base64.encode( srcStd ) );
ifNotEqual( std, enc );
-- invalid decode
ifNotNil( base64.decode( url ) );
ifNotNil( base64.decode( mix1 ) );
ifNotNil( base64.decode( mix2 ) );

-- base64-url
srcUrl = ifNil( base64.decodeURL( url ) );
enc = ifNil( base64.encodeURL( srcUrl ) );
ifNotEqual( url, enc );
-- decode mix
srcUrl = ifNil( base64.decodeMix( url ) );
enc = ifNil( base64.encodeURL( srcUrl ) );
ifNotEqual( url, enc );
-- invalid decode
ifNotNil( base64.decodeURL( std ) );
ifNotNil( base64.decodeURL( mix1 ) );
ifNotNil( base64.decodeURL( mix2 ) );

-- compare decoded std and url
ifNotEqual( srcStd, srcUrl );

-- mixed
srcMix = ifNil( base64.decodeMix( mix1 ) );
enc = ifNil( base64.encode( srcMix ) );
ifNotEqual( enc, std );

srcMix = ifNil( base64.decodeMix( mix2 ) );
enc = ifNil( base64.encodeURL( srcMix ) );
ifNotEqual( enc, url );

-- compare decoded std and url
ifNotEqual( srcStd, srcMix );

