lua-base64mix
=========

base64 encode/decode module

---

## Installation

```sh
luarocks install base64mix --from=http://mah0x211.github.io/rocks/
```


## API

### Encode

#### str, err = base64mix.encode( src:string )

```lua
local base64mix = require('base64mix');
local src = 'hello world';
local enc, err = base64mix.encode( src );
print( enc ); -- 'aGVsbG8gd29ybGQ='
```

#### str, err = base64mix.encodeURL( src:string )

this function will encode an argument to the base64url format.


### Decode

#### str, err = base64mix.decode( src:string )

```lua
local base64mix = require('base64mix');
local src = 'aGVsbG8gd29ybGQ=';
local dec, err = base64mix.decode( src );
print( dec ); -- 'hello world'
```
#### str, err = base64mix.decodeURL( src:string )

this function will decode the base64url format string.

#### str, err = base64mix.decodeMix( src:string )

this function will decode the standard base64 and base64url format string.


