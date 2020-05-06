#ifndef OPTIONS_H
#define OPTIONS_H

#include <boost/optional/optional.hpp>

#include "../structures/types.h"
#include "../strategy/control/types.h"

#include "../util/logger.h"

#include <set>
#include <string>
#include <vector>

struct Options
{
	boost::optional<size_t> threadCount;
	boost::optional<BaselineIOMode> readMode;
	boost::optional<bool> readUVW;
	boost::optional<std::string> strategyFile;
	boost::optional<Logger::VerbosityLevel> logVerbosity;
	boost::optional<bool> skipFlagged;
	boost::optional<std::string> dataColumn;
	boost::optional<std::pair<size_t, size_t>> interval;
	boost::optional<size_t> maxIntervalSize;
	boost::optional<bool> combineSPWs;
	boost::optional<std::string> bandpass;
	std::vector<std::string> preamble;
	std::set<size_t> bands, fields;
	std::set<size_t> antennaeToInclude, antennaeToSkip;
	std::string commandLine;
	std::vector<std::string> filenames;
	enum rfiStrategy::BaselineSelection baselineSelection = rfiStrategy::CrossCorrelations;
};

#endif
