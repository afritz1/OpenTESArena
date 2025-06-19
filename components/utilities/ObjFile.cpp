#include <charconv>
#include <fstream>

#include "Buffer.h"
#include "ObjFile.h"
#include "Span.h"
#include "String.h"
#include "StringView.h"

ObjVertex::ObjVertex()
{
	this->positionX = 0.0;
	this->positionY = 0.0;
	this->positionZ = 0.0;
	this->normalX = 0.0;
	this->normalY = 0.0;
	this->normalZ = 0.0;
	this->texCoordU = 0.0;
	this->texCoordV = 0.0;
}

bool ObjFile::init(const char *filename)
{
	std::ifstream ifs(filename, std::ios::in | std::ios::binary);
	if (!ifs.is_open())
	{
		DebugLogErrorFormat("Couldn't open .OBJ file \"%s\".", filename);
		return false;
	}

	constexpr int positionComponentsPerVertex = 4;
	constexpr int normalComponentsPerVertex = 3;
	constexpr int texCoordComponentsPerVertex = 2;

	std::vector<double> positions;
	std::vector<double> normals;
	std::vector<double> texCoords;

	std::string lineStr;
	int lineNumber = 0;
	while (std::getline(ifs, lineStr))
	{
		lineNumber++;
		if (!lineStr.empty() && lineStr.back() == '\r')
		{
			lineStr.pop_back();
		}

		const Buffer<std::string> lineTokens = String::split(lineStr, ' ');
		if (lineTokens.getCount() == 0)
		{
			continue;
		}

		const std::string &lineType = lineTokens[0];
		if (lineType.empty())
		{
			continue;
		}

		constexpr const char commentSpecifier[] = "#";
		constexpr const char positionSpecifier[] = "v";
		constexpr const char normalSpecifier[] = "vn";
		constexpr const char texCoordSpecifier[] = "vt";
		constexpr const char faceSpecifier[] = "f";
		constexpr const char useMaterialSpecifier[] = "usemtl";
		
		if (lineType == positionSpecifier)
		{
			double positionArray[positionComponentsPerVertex] = { 0.0, 0.0, 0.0, 1.0 };
			for (int i = 1; i < lineTokens.getCount(); i++)
			{
				const std::string &positionToken = lineTokens[i];
				const int positionsIndex = i - 1;
				DebugAssertIndex(positionArray, positionsIndex);
				const std::from_chars_result result = std::from_chars(positionToken.data(), positionToken.data() + positionToken.size(), positionArray[positionsIndex]);
				if (result.ec != std::errc())
				{
					DebugLogErrorFormat("Couldn't parse vertex position in \"%s\" at line %d \"%s\".", filename, lineNumber, lineStr.c_str());
				}
			}

			positions.emplace_back(positionArray[0]);
			positions.emplace_back(positionArray[1]);
			positions.emplace_back(positionArray[2]);
			positions.emplace_back(positionArray[3]);
		}
		else if (lineType == normalSpecifier)
		{
			double normalArray[normalComponentsPerVertex] = { 0.0, 0.0, 0.0 };
			for (int i = 1; i < lineTokens.getCount(); i++)
			{
				const std::string &normalToken = lineTokens[i];
				const int normalsIndex = i - 1;
				DebugAssertIndex(normalArray, normalsIndex);
				const std::from_chars_result result = std::from_chars(normalToken.data(), normalToken.data() + normalToken.size(), normalArray[normalsIndex]);
				if (result.ec != std::errc())
				{
					DebugLogErrorFormat("Couldn't parse vertex normal in \"%s\" at line %d \"%s\".", filename, lineNumber, lineStr.c_str());
				}
			}

			normals.emplace_back(normalArray[0]);
			normals.emplace_back(normalArray[1]);
			normals.emplace_back(normalArray[2]);
		}
		else if (lineType == texCoordSpecifier)
		{
			double texCoordArray[texCoordComponentsPerVertex] = { 0.0, 0.0 };
			for (int i = 1; i < lineTokens.getCount(); i++)
			{
				const std::string &texCoordToken = lineTokens[i];
				const int texCoordsIndex = i - 1;
				DebugAssertIndex(texCoordArray, texCoordsIndex);
				const std::from_chars_result result = std::from_chars(texCoordToken.data(), texCoordToken.data() + texCoordToken.size(), texCoordArray[texCoordsIndex]);
				if (result.ec != std::errc())
				{
					DebugLogErrorFormat("Couldn't parse vertex tex coords in \"%s\" at line %d \"%s\".", filename, lineNumber, lineStr.c_str());
				}
			}

			texCoords.emplace_back(texCoordArray[0]);
			texCoords.emplace_back(texCoordArray[1]);
		}
		else if (lineType == faceSpecifier)
		{
			// Only support one index per position/tex coord/normal tuple for now.
			for (int i = 1; i < lineTokens.getCount(); i++)
			{
				const std::string &indexToken = lineTokens[i];
				int index; // 1-based in .OBJ format.
				const std::from_chars_result result = std::from_chars(indexToken.data(), indexToken.data() + indexToken.size(), index);
				if (result.ec != std::errc())
				{
					DebugLogErrorFormat("Couldn't parse vertex index in \"%s\" at line %d \"%s\".", filename, lineNumber, lineStr.c_str());
				}

				const int zeroBasedIndex = index - 1;
				this->indices.emplace_back(zeroBasedIndex);
			}
		}
		else if (lineType == commentSpecifier)
		{
			// Comment line
		}
		else if (lineType == useMaterialSpecifier)
		{
			if (lineTokens.getCount() != 2)
			{
				DebugLogErrorFormat("Must have one keyword after %s in \"%s\" at line %d \"%s\".", useMaterialSpecifier, filename, lineNumber, lineStr.c_str());
				continue;
			}

			this->materialName = lineTokens[1];
		}
		else
		{
			DebugLogWarningFormat("Unrecognized line type in \"%s\" at line %d \"%s\".", filename, lineNumber, lineStr.c_str());
		}
	}

	// Convert the component arrays into output format for the engine.
	// @todo add vertex deduplication instead of assuming N positions / N normals / N tex coords
	const int vertexCount = static_cast<int>(positions.size()) / positionComponentsPerVertex;
	for (int i = 0; i < vertexCount; i++)
	{
		const int positionComponentsIndex = i * positionComponentsPerVertex;
		const int normalComponentsIndex = i * normalComponentsPerVertex;
		const int texCoordComponentsIndex = i * texCoordComponentsPerVertex;

		ObjVertex vertex;
		vertex.positionX = positions[positionComponentsIndex];
		vertex.positionY = positions[positionComponentsIndex + 1];
		vertex.positionZ = positions[positionComponentsIndex + 2];
		vertex.normalX = normals[normalComponentsIndex];
		vertex.normalY = normals[normalComponentsIndex + 1];
		vertex.normalZ = normals[normalComponentsIndex + 2];
		vertex.texCoordU = texCoords[texCoordComponentsIndex];
		vertex.texCoordV = texCoords[texCoordComponentsIndex + 1];

		this->vertices.emplace_back(std::move(vertex));
	}
	
	return true;
}
