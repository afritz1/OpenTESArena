#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <memory>

// Intended to be a container for the player and world data that is currently active 
// while a player is loaded (i.e., not in the main menu).

// The GameData object in the GameState will be initialized only upon loading of the
// player, and uninitialized when the player goes to the main menu (thus unloading
// the character resources). Whichever entry points into the "game" there are, they
// need to load data into the game data object.

class CLProgram;
class EntityManager;
class Player;

class GameData
{
private:
	std::unique_ptr<Player> player;
	std::unique_ptr<EntityManager> entityManager;
	std::unique_ptr<CLProgram> clProgram;
	double gameTime;
	int32_t worldWidth, worldHeight, worldDepth;
	// province... location... voxels... weather...
	// sprites...
	// date...
public:
	GameData(std::unique_ptr<Player> player,
		std::unique_ptr<EntityManager> entityManager,
		std::unique_ptr<CLProgram> clProgram, double gameTime,
		int32_t worldWidth, int32_t worldHeight, int32_t worldDepth);
	~GameData();

	Player &getPlayer() const;
	EntityManager &getEntityManager() const;
	CLProgram &getCLProgram() const;
	double getGameTime() const;
	int32_t getWorldWidth() const;
	int32_t getWorldHeight() const;
	int32_t getWorldDepth() const;

	void incrementGameTime(double dt);

	// No tick method here.
	// The current panel does what it wants using these methods.
};

#endif
