#include "OptionsUiModel.h"

#include "components/utilities/String.h"

OptionsUiModel::Option::Option(const std::string &name, std::string &&tooltip, OptionType type)
	: name(name), tooltip(std::move(tooltip))
{
	this->type = type;
}

OptionsUiModel::Option::Option(const std::string &name, OptionType type)
	: Option(name, std::string(), type) { }

const std::string &OptionsUiModel::Option::getName() const
{
	return this->name;
}

const std::string &OptionsUiModel::Option::getTooltip() const
{
	return this->tooltip;
}

OptionsUiModel::OptionType OptionsUiModel::Option::getType() const
{
	return this->type;
}

OptionsUiModel::BoolOption::BoolOption(const std::string &name, std::string &&tooltip,
	bool value, Callback &&callback)
	: Option(name, std::move(tooltip), OptionType::Bool), callback(std::move(callback))
{
	this->value = value;
}

OptionsUiModel::BoolOption::BoolOption(const std::string &name, bool value, Callback &&callback)
	: BoolOption(name, std::string(), value, std::move(callback)) { }

std::string OptionsUiModel::BoolOption::getDisplayedValue() const
{
	return this->value ? "true" : "false";
}

void OptionsUiModel::BoolOption::toggle()
{
	this->value = !this->value;
	this->callback(this->value);
}

OptionsUiModel::IntOption::IntOption(const std::string &name, std::string &&tooltip, int value,
	int delta, int min, int max, Callback &&callback)
	: Option(name, std::move(tooltip), OptionType::Int), callback(std::move(callback))
{
	this->value = value;
	this->delta = delta;
	this->min = min;
	this->max = max;
}

OptionsUiModel::IntOption::IntOption(const std::string &name, int value, int delta, int min, int max,
	Callback &&callback)
	: IntOption(name, std::string(), value, delta, min, max, std::move(callback)) { }

int OptionsUiModel::IntOption::getNext() const
{
	return std::min(this->value + this->delta, this->max);
}

int OptionsUiModel::IntOption::getPrev() const
{
	return std::max(this->value - this->delta, this->min);
}

std::string OptionsUiModel::IntOption::getDisplayedValue() const
{
	return (this->displayOverrides.size() > 0) ?
		this->displayOverrides.at(this->value) : std::to_string(this->value);
}

void OptionsUiModel::IntOption::set(int value)
{
	this->value = value;
	this->callback(this->value);
}

void OptionsUiModel::IntOption::setDisplayOverrides(std::vector<std::string> &&displayOverrides)
{
	this->displayOverrides = std::move(displayOverrides);
}

OptionsUiModel::DoubleOption::DoubleOption(const std::string &name, std::string &&tooltip,
	double value, double delta, double min, double max, int precision, Callback &&callback)
	: Option(name, std::move(tooltip), OptionType::Double), callback(std::move(callback))
{
	this->value = value;
	this->delta = delta;
	this->min = min;
	this->max = max;
	this->precision = precision;
}

OptionsUiModel::DoubleOption::DoubleOption(const std::string &name, double value, double delta,
	double min, double max, int precision, Callback &&callback)
	: DoubleOption(name, std::string(), value, delta, min, max, precision, std::move(callback)) { }

double OptionsUiModel::DoubleOption::getNext() const
{
	return std::min(this->value + this->delta, this->max);
}

double OptionsUiModel::DoubleOption::getPrev() const
{
	return std::max(this->value - this->delta, this->min);
}

std::string OptionsUiModel::DoubleOption::getDisplayedValue() const
{
	return String::fixedPrecision(this->value, this->precision);
}

void OptionsUiModel::DoubleOption::set(double value)
{
	this->value = value;
	this->callback(this->value);
}

OptionsUiModel::StringOption::StringOption(const std::string &name, std::string &&tooltip,
	std::string &&value, Callback &&callback)
	: Option(name, std::move(tooltip), OptionType::String), value(std::move(value)),
	callback(std::move(callback)) { }

OptionsUiModel::StringOption::StringOption(const std::string &name, std::string &&value,
	Callback &&callback)
	: StringOption(name, std::string(), std::move(value), std::move(callback)) { }

std::string OptionsUiModel::StringOption::getDisplayedValue() const
{
	return this->value;
}

void OptionsUiModel::StringOption::set(std::string &&value)
{
	this->value = std::move(value);
	this->callback(this->value);
}