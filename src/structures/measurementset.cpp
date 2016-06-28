#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/ms/MeasurementSets/MSTable.h>
#include <casacore/tables/Tables/TableDesc.h>
#include <casacore/tables/Tables/TableRow.h>
#include <casacore/tables/TaQL/ExprNode.h>

#include "measurementset.h"
#include "arraycolumniterator.h"
#include "scalarcolumniterator.h"
#include "date.h"

#include "../util/aologger.h"

#include "../strategy/control/strategywriter.h"

MeasurementSet::~MeasurementSet()
{
}

size_t MeasurementSet::BandCount(const std::string &location)
{
	casacore::MeasurementSet ms(location);
	return ms.spectralWindow().nrow();
}

void MeasurementSet::initializeOtherData()
{
	casacore::MeasurementSet ms(_path);
	_rowCount = ms.nrow();
	initializeAntennas(ms);
	initializeBands(ms);
	initializeFields(ms);
	initializeObservation(ms);
}

void MeasurementSet::initializeAntennas(casacore::MeasurementSet &ms)
{
	casacore::MSAntenna antennaTable = ms.antenna();
	size_t count = antennaTable.nrow();
	casacore::ROArrayColumn<double> positionCol(antennaTable, "POSITION"); 
	casacore::ROScalarColumn<casacore::String> nameCol(antennaTable, "NAME");
	casacore::ROScalarColumn<double> diameterCol(antennaTable, "DISH_DIAMETER");
	casacore::ROScalarColumn<casacore::String> mountCol(antennaTable, "MOUNT");
	casacore::ROScalarColumn<casacore::String> stationCol(antennaTable, "STATION");

	_antennas.resize(count);
	for(size_t row=0; row!=count; ++row)
	{
		AntennaInfo info;
		info.diameter = diameterCol(row);
		info.id = row;
		info.name = nameCol(row);
		casacore::Array<double> position = positionCol(row);
		casacore::Array<double>::iterator i = position.begin();
		info.position.x = *i;
		++i;
		info.position.y = *i;
		++i;
		info.position.z = *i;
		info.mount = mountCol(row);
		info.station = stationCol(row);
		_antennas[row] = info;
	}
}

void MeasurementSet::initializeBands(casacore::MeasurementSet &ms)
{
	casacore::MSSpectralWindow spectralWindowTable = ms.spectralWindow();
	casacore::ROScalarColumn<int> numChanCol(spectralWindowTable, "NUM_CHAN");
	casacore::ROArrayColumn<double> frequencyCol(spectralWindowTable, "CHAN_FREQ");

	_bands.resize(spectralWindowTable.nrow());
	for(size_t bandIndex=0; bandIndex!=spectralWindowTable.nrow(); ++bandIndex)
	{
		BandInfo band;
		band.windowIndex = bandIndex;
		size_t channelCount = numChanCol(bandIndex);

		const casacore::Array<double> &frequencies = frequencyCol(bandIndex);
		casacore::Array<double>::const_iterator frequencyIterator = frequencies.begin();

		for(unsigned channel=0;channel<channelCount;++channel) {
			ChannelInfo channelInfo;
			channelInfo.frequencyIndex = channel;
			channelInfo.frequencyHz = frequencies(casacore::IPosition(1, channel));
			channelInfo.channelWidthHz = 0.0;
			channelInfo.effectiveBandWidthHz = 0.0;
			channelInfo.resolutionHz = 0.0;
			band.channels.push_back(channelInfo);

			++frequencyIterator;
		}

		_bands[bandIndex] = band;
	}
}

void MeasurementSet::initializeFields(casacore::MeasurementSet &ms)
{
	casacore::MSField fieldTable = ms.field();
	casacore::ROArrayColumn<double> delayDirectionCol(fieldTable, fieldTable.columnName(casacore::MSFieldEnums::DELAY_DIR) );
	casacore::ROScalarColumn<casacore::String> nameCol(fieldTable, fieldTable.columnName(casacore::MSFieldEnums::NAME) );

	_fields.resize(fieldTable.nrow());
	for(size_t row=0; row!=fieldTable.nrow(); ++row)
	{
		const casacore::Array<double> &delayDirection = delayDirectionCol(row);
		casacore::Array<double>::const_iterator delayDirectionIterator = delayDirection.begin();
	
		FieldInfo field;
		field.fieldId = row;
		field.delayDirectionRA = *delayDirectionIterator;
		++delayDirectionIterator;
		field.delayDirectionDec = *delayDirectionIterator;
		field.name = nameCol(row);

		_fields[row] = field;
	}
}

void MeasurementSet::initializeObservation(casacore::MeasurementSet& ms)
{
	casacore::MSObservation obsTable = ms.observation();
	casacore::ROScalarColumn<casacore::String> telescopeNameCol(obsTable, obsTable.columnName(casacore::MSObservationEnums::TELESCOPE_NAME));
	if(obsTable.nrow() != 0)
	{
		_telescopeName = telescopeNameCol(0);
		for(size_t row=1; row!=obsTable.nrow(); ++row)
		{
			if(std::string(telescopeNameCol(row)) != _telescopeName)
				throw std::runtime_error("The OBSERVATION table contains multiple entries from different telescopes. I do not know how to handle such sets.");
		}
	}
}

void MeasurementSet::GetDataDescToBandVector(std::vector<size_t>& dataDescToBand)
{
	casacore::MeasurementSet ms(_path);
	casacore::MSDataDescription dataDescTable = ms.dataDescription();
	casacore::ScalarColumn<int>
		spwIdCol(dataDescTable, dataDescTable.columnName(casacore::MSDataDescriptionEnums::SPECTRAL_WINDOW_ID));
	dataDescToBand.resize(dataDescTable.nrow());
	for(size_t dataDescId=0;dataDescId!=dataDescTable.nrow();++dataDescId)
	{
		dataDescToBand[dataDescId] = spwIdCol(dataDescId);
	}
}

MSIterator::MSIterator(class MeasurementSet &ms, bool hasCorrectedData) : _row(0)
{
	_table = new casacore::MeasurementSet(ms.Path());
	_antenna1Col = new casacore::ROScalarColumn<int>(*_table, "ANTENNA1");
	_antenna2Col = new casacore::ROScalarColumn<int>(*_table, "ANTENNA2");
	_dataCol = new casacore::ROArrayColumn<casacore::Complex>(*_table, "DATA");
	_flagCol = new casacore::ROArrayColumn<bool>(*_table, "FLAG");
	if(hasCorrectedData)
		_correctedDataCol = new casacore::ROArrayColumn<casacore::Complex>(*_table, "CORRECTED_DATA");
	else
		_correctedDataCol = 0;
	_fieldCol = new casacore::ROScalarColumn<int>(*_table, "FIELD_ID");
	_timeCol = new casacore::ROScalarColumn<double>(*_table, "TIME");
	_scanNumberCol = new casacore::ROScalarColumn<int>(*_table, "SCAN_NUMBER");
	_uvwCol = new casacore::ROArrayColumn<double>(*_table, "UVW");
	_windowCol = new casacore::ROScalarColumn<int>(*_table, "DATA_DESC_ID");
}

MSIterator::~MSIterator()
{
	delete _antenna1Col;
	delete _antenna2Col;
	delete _dataCol;
	delete _correctedDataCol;
	delete _flagCol;
	delete _timeCol;
	delete _fieldCol;
	delete _table;
	delete _scanNumberCol;
	delete _uvwCol;
	delete _windowCol;
}

void MeasurementSet::initializeMainTableData()
{
	if(!_isMainTableDataInitialized)
	{
		AOLogger::Debug << "Initializing ms cache data...\n"; 
		// we use a ptr to last, for faster insertion
		std::set<double>::iterator obsTimePos = _observationTimes.end();
		MSIterator iterator(*this, false);
		double time = -1.0;
		std::set<std::pair<size_t, size_t> > baselineSet;
		std::set<Sequence> sequenceSet;
		size_t prevFieldId = size_t(-1), sequenceId = size_t(-1);
		for(size_t row=0;row<_rowCount;++row)
		{
			size_t a1 = iterator.Antenna1();
			size_t a2 = iterator.Antenna2();
			size_t fieldId = iterator.Field();
			size_t spw = iterator.Window();
			double cur_time = iterator.Time();
			
			if(fieldId != prevFieldId)
			{
				prevFieldId = fieldId;
				sequenceId++;
				_observationTimesPerSequence.push_back(std::set<double>());
			}
			if(cur_time != time)
			{
				obsTimePos = _observationTimes.insert(obsTimePos, cur_time);
				_observationTimesPerSequence[sequenceId].insert(cur_time);
				time = cur_time;
			}
			
			baselineSet.insert(std::pair<size_t,size_t>(a1, a2));
			sequenceSet.insert(Sequence(a1, a2, spw, sequenceId, fieldId));
			
			++iterator;
		}
		for(std::set<std::pair<size_t, size_t> >::const_iterator i=baselineSet.begin(); i!=baselineSet.end(); ++i)
			_baselines.push_back(*i);
		for(std::set<Sequence>::const_iterator i=sequenceSet.begin(); i!=sequenceSet.end(); ++i)
			_sequences.push_back(*i);
		
		_isMainTableDataInitialized = true;
	}
}

size_t MeasurementSet::PolarizationCount()
{
	return PolarizationCount(Path());
}

size_t MeasurementSet::PolarizationCount(const std::string &filename)
{
	casacore::MeasurementSet ms(filename);
	casacore::Table polTable = ms.polarization();
	casacore::ROArrayColumn<int> corTypeColumn(polTable, "CORR_TYPE"); 
	casacore::Array<int> corType = corTypeColumn(0);
	casacore::Array<int>::iterator iterend(corType.end());
	size_t polarizationCount = 0;
	for (casacore::Array<int>::iterator iter=corType.begin(); iter!=iterend; ++iter)
	{
		++polarizationCount;
	}
	return polarizationCount;
}

bool MeasurementSet::HasRFIConsoleHistory()
{
	casacore::MeasurementSet ms(_path);
	casacore::Table histtab(ms.history());
	casacore::ROScalarColumn<casacore::String> application (histtab, "APPLICATION");
	for(unsigned i=0;i<histtab.nrow();++i)
	{
		if(application(i) == "AOFlagger")
			return true;
	}
	return false;
}

void MeasurementSet::GetAOFlaggerHistory(std::ostream &stream)
{
	casacore::MeasurementSet ms(_path);
	casacore::MSHistory histtab(ms.history());
	casacore::ROScalarColumn<double>       time        (histtab, "TIME");
	casacore::ROScalarColumn<casacore::String> application (histtab, "APPLICATION");
	casacore::ROArrayColumn<casacore::String>  cli         (histtab, "CLI_COMMAND");
	casacore::ROArrayColumn<casacore::String>  parms       (histtab, "APP_PARAMS");
	for(unsigned i=0;i<histtab.nrow();++i)
	{
		if(application(i) == "AOFlagger")
		{
			stream << "====================\n"
				"Command: " << cli(i)[0] << "\n"
				"Date: " << Date::AipsMJDToDateString(time(i)) << "\n"
				"Time: " << Date::AipsMJDToTimeString(time(i)) << "\n"
				"Strategy: \n     ----------     \n";
			const casacore::Vector<casacore::String> appParamsVec = parms(i);
			for(casacore::Vector<casacore::String>::const_iterator j=appParamsVec.begin();j!=appParamsVec.end();++j)
			{
				stream << *j << '\n';
			}
			stream << "     ----------     \n";
		}
	}
}

void MeasurementSet::AddAOFlaggerHistory(const rfiStrategy::Strategy &strategy, const std::string &commandline)
{
	// This has been copied from MSWriter.cc of NDPPP and altered (thanks, Ger!)
	casacore::MeasurementSet ms(_path);
	casacore::Table histtab(ms.keywordSet().asTable("HISTORY"));
	histtab.reopenRW();
	casacore::ScalarColumn<double>       time        (histtab, "TIME");
	casacore::ScalarColumn<int>          obsId       (histtab, "OBSERVATION_ID");
	casacore::ScalarColumn<casacore::String> message     (histtab, "MESSAGE");
	casacore::ScalarColumn<casacore::String> application (histtab, "APPLICATION");
	casacore::ScalarColumn<casacore::String> priority    (histtab, "PRIORITY");
	casacore::ScalarColumn<casacore::String> origin      (histtab, "ORIGIN");
	casacore::ArrayColumn<casacore::String>  parms       (histtab, "APP_PARAMS");
	casacore::ArrayColumn<casacore::String>  cli         (histtab, "CLI_COMMAND");
	// Put all parset entries in a Vector<String>.
	// Some WSRT MSs have a FixedShape APP_PARAMS and CLI_COMMAND column.
	// For them, put the xml file in a single vector element (with newlines).
	bool fixedShaped =
		(parms.columnDesc().options() & casacore::ColumnDesc::FixedShape) != 0;

	std::ostringstream ostr;
	rfiStrategy::StrategyWriter writer;
	writer.WriteToStream(strategy, ostr);

	casacore::Vector<casacore::String> appParamsVec;
	casacore::Vector<casacore::String> clivec;
	clivec.resize(1);
	clivec[0] = commandline;
	if (fixedShaped) {
		appParamsVec.resize(1);
		appParamsVec[0] = ostr.str();
	} else {
		// Tokenize the string on '\n'
		const std::string str = ostr.str();
		size_t lineCount = std::count(str.begin(), str.end(), '\n');
		appParamsVec.resize(lineCount+1);
		casacore::Array<casacore::String>::contiter viter = appParamsVec.cbegin();
		size_t curStringPos = 0;
		for(size_t i=0;i<lineCount;++i)
		{
			size_t endPos = str.find('\n', curStringPos);
			*viter = str.substr(curStringPos, endPos-curStringPos);
			++viter;
			curStringPos = endPos + 1;
		}
		if(curStringPos < str.size())
		{
			*viter = str.substr(curStringPos, str.size()-curStringPos);
		}
	}
	uint rownr = histtab.nrow();
	histtab.addRow();
	time.put        (rownr, casacore::Time().modifiedJulianDay()*24.0*3600.0);
	obsId.put       (rownr, 0);
	message.put     (rownr, "parameters");
	application.put (rownr, "AOFlagger");
	priority.put    (rownr, "NORMAL");
	origin.put      (rownr, "standalone");
	parms.put       (rownr, appParamsVec);
	cli.put         (rownr, clivec);
}

std::string MeasurementSet::GetStationName() const
{
	casacore::MeasurementSet ms(_path);
	casacore::Table antennaTable(ms.antenna());
	if(antennaTable.nrow() == 0)
		throw std::runtime_error("GetStationName() : no rows in Antenna table");
	casacore::ROScalarColumn<casacore::String> stationColumn(antennaTable, "STATION");
	return stationColumn(0);
}

bool MeasurementSet::IsChannelZeroRubish()
{
	try
	{
		const std::string station = GetStationName();
		if(station != "LOFAR") return false;
		// This is of course a hack, but its the best estimate we can make :-/ (easily)
		const BandInfo bandInfo = GetBandInfo(0);
		return (bandInfo.channels.size() == 256 || bandInfo.channels.size()==64);
	} catch(std::exception &e)
	{
		return false;
	}
}
