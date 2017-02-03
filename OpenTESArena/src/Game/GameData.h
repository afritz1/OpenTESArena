#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <memory>
#include <vector>

// Intended to be a container for the player and world data that is currently active 
// while a player is loaded (i.e., not in the main menu).

// The GameData object will be initialized only upon loading of the player, and 
// will be uninitialized when the player goes to the main menu (thus unloading
// the character resources). Whichever entry points into the "game" there are, they
// need to load data into the game data object.

class EntityManager;
class Player;

class GameData
{
private:
	std::unique_ptr<Player> player;
	std::unique_ptr<EntityManager> entityManager;
	std::vector<char> voxelGrid;
	std::vector<char> collisionGrid;
	double gameTime, viewDistance;
	int worldWidth, worldHeight, worldDepth;
	// province... location... weather...
	// date...
public:
	GameData(std::unique_ptr<Player> player,
		std::unique_ptr<EntityManager> entityManager, double gameTime, 
		double viewDistance, int worldWidth, int worldHeight, int worldDepth);
	~GameData();

	Player &getPlayer() const;
	EntityManager &getEntityManager() const;
	std::vector<char> &getVoxelGrid(); // 3D array.
	std::vector<char> &getCollisionGrid(); // 3D array.
	double getGameTime() const;
	double getViewDistance() const;
	int getWorldWidth() const;
	int getWorldHeight() const;
	int getWorldDepth() const;

	void incrementGameTime(double dt);

	// No tick method here.
	// The current panel does what it wants using these methods.
};

#endif
