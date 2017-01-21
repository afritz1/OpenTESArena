# PKLITE V1.12 compression format specification


## Table of contents

* [1 Clarifications](#1-clarifications) 
* [2 Data length](#2-data-length)
  * [2.1 Compressed data](#21-compressed-data)
  * [2.2 Decompressed data](#22-decompressed-data)
    * [2.2.1 First method](#221-first-method)
    * [2.2.2 Second method](#222-second-method)
* [3 Compressed data format](#3-compressed-data-format)
  * [3.1 Data description](#31-data-description)
  * [3.2 Encrypted data](#32-encrypted-data)
  * [3.3 Relocation data](#33-relocation-data)
* [4 Decompression](#4-decompression)
  * [4.1 Decision](#41-decision)
  * [4.2 Decryption](#42-decryption)
  * [4.3 Duplication](#43-duplication)
    * [4.3.1 Number of bytes](#431-number-of-bytes)
      * [4.3.1.1 Special case](#4311-special-case)
    * [4.3.2 Offset](#432-offset)
    * [4.3.3 Duplicating data](#433-duplicating-data)
* [5 Pointer relocation](#5-pointer-relocation)


----------


## 1 Clarifications

This specification should work with any executable compressed with PKLITE V1.12, but it is mainly designed for 'A.EXE' from Bethesda's 'The Elder Scrolls: Arena'.


----------


## 2 Data length

### 2.1 Compressed data

The compressed data is stored within the executable in little endian format.

If `l` is the length of the executable in bytes, then the compressed data is stored from byte at position `0x2F0` up until `l - 8` within the executable. The compressed data should end with `0xFFFF`.

### 2.2 Decompressed data

It may be useful to know how much space is required to store the decompressed data. The methods below describe ways to do it.

#### 2.2.1 First method

It can be estimated by loading in the 2-byte value at position `0x61`, and then performing the following operation on it:

`estimated length = value * 0x10 - 0x450`

This will give a slight overestimate for the actual length of decompressed data.


#### 2.2.2 Second method

In case of 'A.EXE', the length of decompressed data can be calculated more precisely.

When 'A.EXE' finishes decompressing the data, it places the stack pointer at a pre-determined position, which happens to be at the end of the decompressed data. So, if one can calculate where the stack pointer would be placed, one will know how long the decompressed data is.

**Side note:** This is the reason why the decompressed data from A.EXE has empty space at the end: to leave room for the stack.

The position of the stack pointer can be calculated using the 'segment' stored at position `l - 8`, and the 'offset' stored at `l - 6`. Both of those values are 2 bytes long. Using these two values, the length of the decompressed data can be calculated, like so:

`length = segment * 0x10 + offset`

In case of 'A.EXE', `segment = 0x4A57` and `offset = 0x0080`, so the length of decompressed data is `0x4A5F0` bytes.

This might not work with other executables compressed with PKLITE V1.12, since their stack pointer might not be placed at end of the decompressed data, but instead somewhere else. It's still possible to calculate an overestimate for them using the first method.


----------


## 3 Compressed data format

The compressed data is continuous, so it should be read from start to finish continuously, i.e. no going back and forth. If something is to be read from the compressed data, it should be read from where the last 'read operation' left off.

This also applies to storing decompressed data: all new data should be appended to the end of existing decompressed data (or the beginning if no data has been decompressed yet).

### 3.1 Data description

The compressed data contains a long stream of bits that describes how the data should be decompressed.

Unfortunately, that stream has been split up into chunks, which are scattered all around the data, and it isn't clear how many of them there are.

Fortunately, the first chunk is always at the start of the compressed data, and the rest can be found based on the contents of the previous chunks.

These chunks are stored in little endian format, and they are 2 bytes long, meaning that they contain 16 bits.

The bits those chunks contain are going to be checked/processed sequentially. The order is important. The first bit of the chunk is the least significant bit of the 2 byte value. The last (16th) bit of the chunk is the most significant bit.

So, for example, if the 2 byte value was `0xC41C` (`1100010000011100b`), the first bit of the chunk would be `0`, and `1` would be the last (16th) bit of the chunk.

During decompression, bits from the 'data description' bit stream will be requested. The flowchart below shows how it should be handled:

```
   +---------+
   |   Bit   |
   |requested|
   +---------+
        |
        v
  +----------+
  | Load bit |
  |from chunk|
  +----------+
        |
        v
  +------------+
  |Was this the|  Yes   +--------+
  |last bit in |------->|Load new|
  |   chunk?   |        | chunk  |
  +------------+        +--------+
        |                   |
        | No                v
        v             +------------+
  +-----------+       |  Point to  |
  |Advance bit|       |first bit in|
  |  pointer  |       | new chunk  |
  +-----------+       +------------+
        |                   |
        +-------------------+
        |
        v
   +----+-----+
   |  Output  |
   |loaded bit|
   +----------+
```

So, even though the bit stream has been split up into chunks, individual bits can be loaded from it in a continuous and seamless way.

Each new chunk should be loaded in from compressed data, like all other data.

### 3.2 Encrypted data

There is encrypted data stored between the chunks. It will be loaded in and decrypted when necessary as described in '[4.2 Decryption](#42-decryption)'.

### 3.3 Relocation data

The decompression process will finish before all of the compressed data is processed. The rest of the compressed data describes how the pointers within decompressed data should be relocated. This process will be described in '[5 Pointer relocation](#5-pointer-relocation)'.


----------


## 4 Decompression

At the start of decompression, the first chunk should be loaded in. The 'bit pointer' should point to the first bit in this chunk.

### 4.1 Decision

A bit should be read from the 'data description' bit stream, as described previously.

* If the bit is a `0`, then a single byte from the compressed data should be decrypted. This will be explained further in '[4.2 Decryption](#42-decryption)'.
* If the bit is a `1`, then a part of the decompressed data should be duplicated. This will be explained further in '[4.3 Duplication](#43-duplication)'.

Then, once an appropriate action has been done, another [Decision](#41-decision) should be made, followed by the right action.

This should be repeated until all of the compressed data has been processed, or until decompression has finished, which will be described in '[4.3.1.1 Special case](#4311-special-case)'.


----------


### 4.2 Decryption

When decrypting, a byte from the compressed data is loaded in, decrypted and placed in decompressed data.

The byte is encrypted with an XOR cipher. It can be decrypted by performing an XOR operation on the byte, with the right key.

The key is the number of unread bits in the current chunk, so it can range from 16 to 1. It can't be 0, because when all bits have been read from a chunk, a new chunk should be loaded in, which will have 16 unread bits.


----------


### 4.3 Duplication

When duplicating, a part of the existing decompressed data is duplicated and appended to the end.

In order to do that, we need to know how many bytes to copy, and where from.

#### 4.3.1 Number of bytes

Once it has been decided that decompressed data needs to be duplicated, a few bits from the bit stream will need to be loaded in. These bits will determine the number of bytes that should be duplicated.

However, it is not certain how many bits need to be loaded in, because the values are encoded using a prefix code, which is variable-length. This is done to compress them.

The table below shows how codewords should be decoded. The leftmost bit is the first bit read, the second one is the second bit read, and so on.

The decimal values are provided for convinience, as they don't have to (and maybe shouldn't) be used in decoding.

If the table looks confusing, I would recommend reading about prefix codes, Huffman codes, or even Shannon–Fano codes.

|   Binary  | Decimal | Decoded |
|-----------|:-------:|:-------:|
| 10        |    2    |    2    |
| 11        |    3    |    3    |
| 000       |    0    |    4    |
| 0010      |    2    |    5    |
| 0011      |    3    |    6    |
| 0100      |    4    |    7    |
| 01010     |    10   |    8    |
| 01011     |    11   |    9    |
| 01100     |    12   |    10   |
| 011010    |    26   |    11   |
| 011011    |    27   |    12   |
| 011100    |    28   | Special |
| 0111010   |    58   |    13   |
| 0111011   |    59   |    14   |
| 0111100   |    60   |    15   |
| 01111010  |   122   |    16   |
| 01111011  |   123   |    17   |
| 01111100  |   124   |    18   |
| 011111010 |   250   |    19   |
| 011111011 |   251   |    20   |
| 011111100 |   252   |    21   |
| 011111101 |   253   |    22   |
| 011111110 |   254   |    23   |
| 011111111 |   255   |    24   |

If a codeword consists of only 2 bits, then only 2 bits should be loaded in to decode it. This is possible due to the properties of prefix codes, where no codeword is the prefix of another codeword.

The decoded value is the number of bytes that Duplication should copy.

##### 4.3.1.1 Special case

If the bits decode into the 'Special case', then a byte should be read from compressed data:

* If the byte is `0xFE`, then Duplication should be aborted, and a [Decision](#41-decision) should be made again.
* If the byte is `0xFF`, then Duplication should be aborted, and **the decompression process is finished**. One can now proceed with the relocation process, described at '[5 Pointer relocation](#5-pointer-relocation)'.
* Otherwise, the number of bytes that Duplication should copy is `byte + 25`.


----------


#### 4.3.2 Offset

If Duplication wasn't aborted, then the number of bytes to copy was either decoded or loaded in. Now one should find the position of the bytes that are to be copied.

The position will be found as a backward offset from the end of existing compressed data. The offset will specify the beginning of the data to copy.

For example, if the offset is 5, then the 5th to last byte would be copied, assuming only 1 byte was to be copied.

The offset is a 2-byte value, with a less significant byte and a more significant byte. The more significant byte will be found first.

If the number of bytes to be copied (found previously) is 2, then the more significant byte is going to be 0, and no decoding should be done.

Otherwise, the more significant byte of the offset has to be decoded. The table below shows how to decode it:

|  Binary | Decimal | Decoded |
|---------|:-------:|:-------:|
| 1       |    1    |    0    |
| 0000    |    0    |    1    |
| 0001    |    1    |    2    |
| 00100   |    4    |    3    |
| 00101   |    5    |    4    |
| 00110   |    6    |    5    |
| 00111   |    7    |    6    |
| 010000  |    16   |    7    |
| 010001  |    17   |    8    |
| 010010  |    18   |    9    |
| 010011  |    19   |    10   |
| 010100  |    20   |    11   |
| 010101  |    21   |    12   |
| 010110  |    22   |    13   |
| 0101110 |    46   |    14   |
| 0101111 |    47   |    15   |
| 0110000 |    48   |    16   |
| 0110001 |    49   |    17   |
| 0110010 |    50   |    18   |
| 0110011 |    51   |    19   |
| 0110100 |    52   |    20   |
| 0110101 |    53   |    21   |
| 0110110 |    54   |    22   |
| 0110111 |    55   |    23   |
| 0111000 |    56   |    24   |
| 0111001 |    57   |    25   |
| 0111010 |    58   |    26   |
| 0111011 |    59   |    27   |
| 0111100 |    60   |    28   |
| 0111101 |    61   |    29   |
| 0111110 |    62   |    30   |
| 0111111 |    63   |    31   |

There is no special case for this one.

Once the more significant byte has been found, a byte should be loaded in from the compressed data. The loaded byte is the less significant byte of the offset.

These two bytes combine into a 2-byte number, which is the offset. It can be used along with the 'number of bytes' (found previously) to duplicate decompressed data.


----------


#### 4.3.3 Duplicating data

Once the 'number of bytes' and the offset have been found, the decompressed data can be duplicated.

If `l` is the length of the existing decompressed data, then decompressed data from `l - offset` to `l - offset + number of bytes` should be copied and appended in front of the existing decompressed data.


----------


## 5 Pointer relocation

Once the decompression is finished, the pointers within the decompressed code can be relocated, so that they point to the right address.

However, this has little practical use outside of DOS, plus the strings/data are left unchanged.

It might be even better to leave the pointers as they are, since they will remain relative to the start decompressed data.

As such, I won't explain how to relocate the pointers. I'm sorry if you really needed to do it.
