#pragma once

#include <QObject>

class Range
{
	Q_GADGET
	Q_PROPERTY(int min MEMBER min)
	Q_PROPERTY(int max MEMBER max)

public:
	Range(int min_, int max_)
		: min(min_)
		, max(max_)
	{}

	Range() = default;

	int min;
	int max;
};

Q_DECLARE_METATYPE(Range)