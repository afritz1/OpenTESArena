#ifndef TEXT_EVENTS_H
#define TEXT_EVENTS_H

#include <functional>
#include <string>

// When an alphanumeric key press or hold occurs during text entry mode.
using TextInputCallback = std::function<void(const std::string &text)>;

#endif
