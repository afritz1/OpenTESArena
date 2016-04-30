#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <memory>

// Intended to be a container for the player and world data that is currently active 
// while a player is loaded (i.e., not in the main menu).

// The GameData object in the GameState will be initialized only upon loading of the
// player, and uninitialized when the player goes to the main menu (thus unloading
// the character resources). Whichever entry points into the "game" there are, they
// need to load data into the game data object.

class EntityManager;
class Player;

class GameData
{
private:
	std::unique_ptr<Player> player;
	std::unique_ptr<EntityManager> entityManager;
	// province... location... voxels... weather...
	// sprites...
	// date... time...
public:
	GameData(std::unique_ptr<Player> player, 
		std::unique_ptr<EntityManager> entityManager);
	~GameData();

	Player &getPlayer() const;
	EntityManager &getEntityManager() const;

	// No tick method; the current panel does what it wants using these methods.
};

#endif
