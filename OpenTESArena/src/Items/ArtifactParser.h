#ifndef ARTIFACT_PARSER_H
#define ARTIFACT_PARSER_H

#include <memory>
#include <vector>

class ArtifactData;

enum class ProvinceName;

class ArtifactParser
{
private:
	static const std::string PATH;
	static const std::string FILENAME;
	
	ArtifactParser() = delete;
	ArtifactParser(const ArtifactParser&) = delete;
	~ArtifactParser() = delete;

	static std::vector<ProvinceName> parseProvinces(
		const std::vector<std::string> &provinceTokens);

	// Add "makeConsumable" and "makeTrinket" methods later if adding custom artifacts.
	static std::unique_ptr<ArtifactData> makeAccessory(const std::string &displayName,
		const std::string &description, const std::vector<ProvinceName> &provinces,
		const std::string &accessoryTypeToken, const std::string &metalToken);
	static std::unique_ptr<ArtifactData> makeBodyArmor(const std::string &displayName, 
		const std::string &description, const std::vector<ProvinceName> &provinces,
		const std::string &partNameToken, const std::string &materialToken);
	static std::unique_ptr<ArtifactData> makeMiscellaneous(const std::string &displayName, 
		const std::string &description, const std::vector<ProvinceName> &provinces,
		const std::string &miscTypeToken);
	static std::unique_ptr<ArtifactData> makeShield(const std::string &displayName, 
		const std::string &description, const std::vector<ProvinceName> &provinces,
		const std::string &shieldTypeToken, const std::string &metalToken);
	static std::unique_ptr<ArtifactData> makeWeapon(const std::string &displayName, 
		const std::string &description, const std::vector<ProvinceName> &provinces,
		const std::string &weaponTypeToken, const std::string &metalToken);
public:
	static std::vector<std::unique_ptr<ArtifactData>> parse();
};

#endif
