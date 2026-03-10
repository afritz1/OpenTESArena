#ifndef LOGBOOK_PANEL_H
#define LOGBOOK_PANEL_H

#include "Panel.h"

class LogbookPanel : public Panel
{
public:
	LogbookPanel(Game &game);
	~LogbookPanel() override;

	bool init();
};

#endif
