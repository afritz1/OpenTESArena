# PKLITE V1.12 compression format specification


This specification should work with any executable compressed with PKLITE V1.12, but it is mainly designed for A.EXE from Bethesda's 'The Elder Scrolls: Arena'.



## Length of compressed/decompressed data

If `l` is the length of the executable in bytes, then the compressed data is stored from position `0x2F0` up until `l - 8` within the executable. The compressed data should end with the word `0xFFFF`.

However, it isn't entirely certain how long the *decompressed* data is going to be.


### First method

It can be estimated by loading in the word at position `0x61`, and then performing the following operation on it:

`estimated length = word * 0x10 - 0x450`

This will give a slight overestimate for the actual length of decompressed data.


### Second method

In case of A.EXE, the length of decompressed data can be calculated more precisely. This method relies on using the position that the stack pointer would be given at the end of decompression, if the program were run in DOS. In case of A.EXE, the stack pointer is placed at the end of the decompressed data. So, if one can calculate its position, one will know the length of decompressed data.

**Side note:** This is the reason why the decompressed data from A.EXE has empty space at the end: to leave room for the stack.

The position of the stack pointer can be calculated using the 'segment' stored at position `l - 8`, and the 'offset' stored at `l - 6`. Both of them are 2 bytes long.

Using these two values, the length of the decompressed data can be calculated, like so:

`length = segment * 0x10 + offset`

In case of A.EXE, `segment = 0x4A57` and `offset = 0x0080`, so the length of decompressed data is `0x4A5F0` bytes.

This might not work with other executables compressed with PKLITE V1.12, since their stack might not be located at the end of the decompressed data, but instead somewhere else. It's still possible to calculate an overestimate for them using the first method.



## Compressed data format

All of the data stored within the executable is in little endian format. When data is being decompressed, it should be stored in little endian as well.
Since the data is likely to be accessed at 1 byte intervals, little endianness will only make a difference with values longer than 1 byte.

The compressed data is continuous, and it should be read from start to finish continuously, i.e. no going back and forth. If something is to be read from the compressed data, it should be read from where the last 'read operation' left off.
This also applies to storing decompressed data: all new data should be appended to the end of existing decompressed data (or the beginning if no data has been decompressed yet).

The compressed data consists of bit arrays with various data between them. The first bit array is always located at the beginning of the compressed data. The position of the rest of the bit arrays is unknown, and will have to be determined at runtime.
These 16-bit bit arrays describe how the compressed data is to be decompressed. Their contents are used to decide what compressed/uncompressed data to load, modify and copy.

New bit arrays are expected to be loaded in as soon as the old ones are fully processed, and since other data may be loaded in the meantime, their position will vary.
These 16-bit arrays are stored as a single little endian word. When it is required to load a bit array, this word should be read in, from which the array can be constructed.

The LSB (least significant bit) is the first bit of the bit array. The MSB (most significant bit) is the last bit of the bit array. So, for example, if the word is `0xD132` (which is `1101000100110010b`):

|  Bits |   1   |   1   |   0   |   1   |   0   |   0   |   0   |   1   |   0   |   0   |   1   |   1   |   0   |   0   |   1   |   0   |
|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|
| Order |  16th |  15th |  14th |  13th |  12th |  11th |  10th |  9th  |  8th  |  7th  |  6th  |  5th  |  4th  |  3rd  |  2nd  |  1st  |

`0` is the first bit of the bit array, `1` is the second bit, and so on, meaning that the 15th bit is a `1` and the 16th bit is a `1`.

Bits from the bit array will be requested during the decompression process. Like with compressed data, bits should be read continuously from the bit array.
Each bit should only be read/checked/accessed once.
If after reading a bit there are no more unread bits left in the array, a new array should be loaded in. This is done so that there are always bits ready.
The new bit array should replace the old one.

On a newly loaded array, the first bit should be read first, then the second, and so on.

The bit array should persist across all modes.

When the decompression starts, the first bit array (located at the beginning of compressed data) should be loaded in, and the decompression enters a '[Default Mode](#default-mode)'.



## Default Mode

During the Default Mode, a single bit is read from the bit array.

* If the bit is a 0, then Decrypt Mode should be entered, where a byte of compressed data is decrypted and transferred to decompressed data. This will be explained further in '[Decrypt Mode](#decrypt-mode)'.
* If the bit is a 1, then Copy Mode should be entered, where a part of the decompressed data is duplicated. This will be explained further in '[Copy Mode](#copy-mode)'.

Then, once the appropriate action has been done, the Default Mode repeats. It should be repeated as long as there is data available to read, or until the decompression finishes, which will be described in '[Copy Mode](#copy-mode)'.



## Decrypt Mode

During the Decrypt Mode, the next *byte* from compressed data should be read.

This byte is encrypted with an XOR cipher. It can be decrypted by performing an XOR operation on the byte, with the right key.

The key varies, but it follows a pattern. It depends on which bit within the bit array caused the Decrypt Mode to be entered. The table below shows the bit positions with their equivalent keys.

| Bit position |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10 |  11 |  12 |  13 |  14 |  15 |  16 |
|:------------:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|     Key      |  15 |  14 |  13 |  12 |  11 |  10 |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  16 |

Keep in mind that the key for the 16th bit is 16, not 0.

Once an XOR operation has been performed on the byte (with the correct key), the decrypted byte should be stored in the decompressed data.

When this is done, Default Mode should be re-entered.



## Copy Mode

The Copy Mode duplicates some of the existing decompressed data and appends it to the end of the decompressed data.

To do that, it needs to figure out how many bytes to copy, and where from.


### Number of bytes

The number of bytes that the Copy Mode will copy can be found by reading in bits from the bit arrays.

The bits form a codeword of a specifically designed 'Huffman code'.
The table below shows how the codewords are to be decoded:

|   Binary  | Decimal | Decoded |
|-----------|:-------:|:-------:|
| 10        |    2    |    2    |
| 11        |    3    |    3    |
| 000       |    0    |    4    |
| 0010      |    2    |	   5    |
| 0011      |    3    |    6    |
| 0100      |    4    |	   7    |
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

The task of decoding these numbers can be done in many ways. In any case, as few bits should be read as possible. If the bits match any of the binary numbers, then the number is decoded.

The decoded number is the number of bytes to be copied in Copy Mode.

#### Further explanation
One could start off by reading in two bits from the bit arrays, and checking if they matches with any of the top two options:
* If it does (i.e. the first bit read was a 1), then the number is decoded. No more bytes should be read.
* If it doesn't (i.e. the first bit read wasn't a 1), then the third bit should be loaded in.

Then the bits are checked against the 3rd option. If it doesn't match, the next bit is loaded in. Then they are checked against the next 3 options. And so on.

This might be a naive way to decode the numbers, but it is quick, easy, and clear. One could also try to decode it with a 'Huffman tree' or something else. As long as the bits are decoded correctly, it doesn't matter.

#### Special case
If the bits decode into the 'Special case', then a single byte should be read from compressed data:
* If the byte is `0xFE`, then the Copy Mode should be aborted, and Default Mode should commence.
* If the byte is `0xFF`, then the Copy Mode should be aborted, and **the decompression process is finished**. One can now proceed with the relocation process, described at '[Pointer relocation](pointer-relocation)'.
* Otherwise, the number of bytes to be copied in Copy Mode is equal to `byte + 25`. This is used to allow more than 24 bytes to be copied.


### Position

If Copy Mode wasn't aborted, then the number of bytes to copy was either decoded or loaded in. Now one can find the position of the bytes that are to be copied.
The position will be specified by an offset going back through the decompressed data. The offset will specify the beginning of the data to copy.

The offset is a two byte value, with a less significant byte and a more significant byte. The more significant byte will be found first.

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

These two bytes combine into a 2 byte number, which is the offset. It can be used along with the 'number of bytes' to duplicate decompressed data.


### Duplicating data

Once the 'number of bytes' and the offset have been found, the decompressed data can be duplicated.

If `l` is the length of the existing decompressed data, then decompressed data from `l - offset` to `l - offset + number` should be copied and appended in front of the existing decompressed data.

Once it is copied, the Default Mode should be re-entered.



## Pointer relocation

Once the decompression is finished, the pointers within the decompressed code can be relocated, so that they point to the right address.

However, this only has to be done if you want to execute the code and you are running it in DOS.

The strings/data are left unchanged.

I could explain how to do it, but I don't see the point.
