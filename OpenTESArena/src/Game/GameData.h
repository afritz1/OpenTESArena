#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <memory>
#include <string>
#include <vector>

#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../World/VoxelGrid.h"

// Intended to be a container for the player and world data that is currently active 
// while a player is loaded (i.e., not in the main menu).

// The GameData object will be initialized only upon loading of the player, and 
// will be uninitialized when the player goes to the main menu (thus unloading
// the character resources). Whichever entry points into the "game" there are, they
// need to load data into the game data object.

class CharacterClass;
class Renderer;
class TextureManager;

enum class GenderName;

class GameData
{
private:
	// The number of real-time seconds that an in-game day lasts.
	static const double SECONDS_PER_DAY;

	Player player;
	EntityManager entityManager;
	VoxelGrid voxelGrid;
	double gameTime, fogDistance;
	// province... location... weather...
	// date...
public:
	GameData(Player &&player, EntityManager &&entityManager, VoxelGrid &&voxelGrid,
		double gameTime, double fogDistance);
	~GameData();

	// Creates a game data object used for the test world.
	static std::unique_ptr<GameData> createDefault(const std::string &playerName,
		GenderName gender, int raceID, const CharacterClass &charClass,
		int portraitID, TextureManager &textureManager, Renderer &renderer);

	Player &getPlayer();
	EntityManager &getEntityManager();
	VoxelGrid &getVoxelGrid();
	double getGameTime() const;

	// Gets a 0->1 value representing how far along the current day is. 
	// 0.0 is 12:00am and 0.50 is noon.
	double getDaytimePercent() const;

	double getFogDistance() const;

	void incrementGameTime(double dt);

	// No tick method here.
	// The current panel does what it wants using these methods.
};

#endif
