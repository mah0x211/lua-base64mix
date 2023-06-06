local assert = require('assert')
local randstr = require('string.random')
local base64 = require('base64mix')

local function test_encode_decode_std()
    for _ = 1, 100 do
        local src = randstr(50)

        -- test that encode a string to [a-zA-Z0-9+/=]
        local enc = assert(base64.encode(src))
        assert.match(enc, '^[a-zA-Z0-9/=+]+$', false)

        -- test that decode a string to original string
        local dec = assert(base64.decode(enc))
        assert.equal(dec, src)

        -- test that decodeURL cannot decode a string except [a-zA-Z0-9_-=]
        if not string.find(enc, '^[a-zA-Z0-9_=-]+$') then
            local _, err = base64.decodeURL(enc)
            assert.is_nil(_)
            assert.re_match(err, 'invalid argument', 'i')
        end
    end
end
test_encode_decode_std()

local function test_encode_decode_url()
    for _ = 1, 100 do
        local src = randstr(50)

        -- test that encode a string to [a-zA-Z0-9_-]
        local enc = assert(base64.encodeURL(src))
        assert.match(enc, '^[a-zA-Z0-9_-]+$', false)

        -- test that decode a string to original string
        local dec = assert(base64.decodeURL(enc))
        assert.equal(dec, src)

        -- test that decode cannot decode a string except [a-zA-Z0-9+/=]
        if string.find(enc, '[_-]') then
            local _, err = base64.decode(enc)
            assert.is_nil(_)
            assert.re_match(err, 'invalid argument', 'i')
        end
    end
end
test_encode_decode_url()

local function test_decode_mix()
    for _ = 1, 100 do
        local src = randstr(50)
        local enc_std = assert(base64.encode(src))
        assert.match(enc_std, '^[a-zA-Z0-9/=+]+$', false)
        local enc_url = assert(base64.encodeURL(src))
        assert.match(enc_url, '^[a-zA-Z0-9_-]+$', false)

        -- test that decodeMix can decode both standard and url encoded strings
        assert.equal(base64.decodeMix(enc_std), src)
        assert.equal(base64.decodeMix(enc_url), src)
    end
end
test_decode_mix()

local function test_decode_invalid_padding()
    -- test that return error if invalid padding
    local _, err = base64.decode('invalid=+')
    assert.is_nil(_)
    assert.re_match(err, 'invalid argument', 'i')
end
test_decode_invalid_padding()
