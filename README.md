lua-base64mix
=========

[![test](https://github.com/mah0x211/lua-base64mix/actions/workflows/test.yml/badge.svg)](https://github.com/mah0x211/lua-base64mix/actions/workflows/test.yml)
[![codecov](https://codecov.io/gh/mah0x211/lua-base64mix/branch/master/graph/badge.svg)](https://codecov.io/gh/mah0x211/lua-base64mix)

base64 encode/decode module.

---

## Installation

```sh
luarocks install base64mix
```


## str, err = base64mix.encode( src:string )

```lua
local base64mix = require('base64mix')
local src = 'hello world'
local enc, err = base64mix.encode(src)
print(enc) -- 'aGVsbG8gd29ybGQ='
```

## str, err = base64mix.encodeURL( src:string )

this function encodes a string into a URL-safe base64 format string.


## str, err = base64mix.decode( src:string )

```lua
local base64mix = require('base64mix')
local src = 'aGVsbG8gd29ybGQ='
local dec, err = base64mix.decode(src)
print(dec) -- 'hello world'
```

## str, err = base64mix.decodeURL( src:string )

this function decodes a string in URL-safe base64 format.

## str, err = base64mix.decodeMix( src:string )

this function decodes both standard and URL-safe base64 format.


