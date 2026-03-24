#ifndef PANEL_H
#define PANEL_H

class Game;

// Each panel interprets user input and draws to the screen. There is only one panel active at
// a time, and it is owned by Game, although there can be any number of sub-panels.
class Panel
{
private:
	Game &game;
protected:
	Game &getGame() const;
public:
	Panel(Game &game);
	virtual ~Panel();
};

#endif
