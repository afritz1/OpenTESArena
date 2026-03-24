#include "Panel.h"

Panel::Panel(Game &game)
	: game(game) { }

Panel::~Panel()
{

}

Game &Panel::getGame() const
{
	return this->game;
}
