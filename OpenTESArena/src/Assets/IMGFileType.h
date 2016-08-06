#ifndef IMG_FILE_TYPE_H
#define IMG_FILE_TYPE_H

// A unique identifier for each type of IMG file format.

// "Normal" IMG files have a header that determines their properties.
// "Raw" IMG files have no header, so the properties must be hardcoded.
// "Wall" IMG files have no header, and their dimensions are 64x64.

enum class IMGFileType
{
	Normal,
	Raw,
	Wall
};

#endif
