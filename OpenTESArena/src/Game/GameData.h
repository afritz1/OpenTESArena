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

enum class GenderName;

class GameData
{
private:
	Player player;
	EntityManager entityManager;
	VoxelGrid voxelGrid;
	std::vector<char> collisionGrid;
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
		int portraitID);

	Player &getPlayer();
	EntityManager &getEntityManager();
	VoxelGrid &getVoxelGrid();
	std::vector<char> &getCollisionGrid(); // 3D array.
	double getGameTime() const;
	double getFogDistance() const;

	void incrementGameTime(double dt);

	// No tick method here.
	// The current panel does what it wants using these methods.
};

#endif
