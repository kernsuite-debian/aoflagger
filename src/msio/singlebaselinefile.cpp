#include "singlebaselinefile.h"

#include "../util/serializable.h"

#include "../version.h"

#define FILE_FORMAT_VERSION 1

void SingleBaselineFile::Read(std::istream& stream)
{
	char magic[9];
	stream.read(magic, 8);
	magic[8] = 0;
	if(std::string(magic) != "RFIBL")
		throw std::runtime_error("This is not a AOFlagger single baseline file");
	unsigned fileformat = Serializable::UnserializeUInt32(stream);
	if(fileformat != FILE_FORMAT_VERSION)
		throw std::runtime_error("This AOFlagger single baseline file has an unknown file format");
	std::string versionStr = Serializable::UnserializeString(stream);
	Serializable::UnserializeUInt32(stream); // maj
	Serializable::UnserializeUInt32(stream); // min
	Serializable::UnserializeUInt32(stream); // submin
	
	data = UnserializeTFData(stream);
	metaData = UnserializeMetaData(stream);
	
	telescopeName = Serializable::UnserializeString(stream);
}

void SingleBaselineFile::Write(std::ostream& stream)
{
	stream.write("RFIBL\0\0\0", 8);
	Serializable::SerializeToUInt32(stream, FILE_FORMAT_VERSION); // fileformat version index
	Serializable::SerializeToString(stream, AOFLAGGER_VERSION_DATE_STR);
	Serializable::SerializeToUInt32(stream, AOFLAGGER_VERSION_MAJOR);
	Serializable::SerializeToUInt32(stream, AOFLAGGER_VERSION_MINOR);
	Serializable::SerializeToUInt32(stream, AOFLAGGER_VERSION_SUBMINOR);
	
	Serialize(stream, data);
	Serialize(stream, metaData);
	
	Serializable::SerializeToString(stream, telescopeName);
}

TimeFrequencyData SingleBaselineFile::UnserializeTFData(std::istream& stream)
{
	TimeFrequencyData data;
	size_t polCount = Serializable::UnserializeUInt32(stream);
	size_t complCode = Serializable::UnserializeUInt32(stream);
	enum TimeFrequencyData::ComplexRepresentation repr;
	switch(complCode)
	{
		default:
		case 0: repr = TimeFrequencyData::PhasePart; break;
		case 1: repr = TimeFrequencyData::AmplitudePart; break;
		case 2: repr = TimeFrequencyData::RealPart; break;
		case 3: repr = TimeFrequencyData::ImaginaryPart; break;
		case 4: repr = TimeFrequencyData::ComplexParts; break;
	}
	for(size_t i=0; i!=polCount; ++i)
	{
		TimeFrequencyData polData;
		size_t polCode = Serializable::UnserializeUInt32(stream);
		PolarizationEnum pol = Polarization::AipsIndexToEnum(polCode);
		uint32_t imageFlagBitset = Serializable::UnserializeUInt32(stream);
		size_t imageCount = imageFlagBitset & 0x03;
		size_t maskCount = (imageFlagBitset & 0x04) ? 1 : 0;
		if(imageCount == 2)
		{
			Image2D
				first = UnserializeImage(stream),
				second = UnserializeImage(stream);
			polData = TimeFrequencyData(pol, Image2D::MakePtr(first), Image2D::MakePtr(second));
		}
		else if(imageCount == 1)
		{
			polData = TimeFrequencyData(repr, pol, Image2D::MakePtr(UnserializeImage(stream)));
		}
		if(maskCount == 1)
			polData.SetGlobalMask(Mask2D::MakePtr(UnserializeMask(stream)));
		if(i==0)
			data = polData;
		else
			data = TimeFrequencyData::MakeFromPolarizationCombination(data, polData);
	}
	return data;
}

TimeFrequencyMetaData SingleBaselineFile::UnserializeMetaData(std::istream& stream)
{
	TimeFrequencyMetaData metaData;
	size_t featureSet = Serializable::UnserializeUInt64(stream);
	bool hasAntenna1 = featureSet & 0x01;
	bool hasAntenna2 = featureSet & 0x02;
	bool hasBand = featureSet & 0x04;
	bool hasObsTimes = featureSet & 0x10;
	if(hasAntenna1)
	{
		AntennaInfo ant;
		ant.Unserialize(stream);
		metaData.SetAntenna1(ant);
	}
	if(hasAntenna2)
	{
		AntennaInfo ant;
		ant.Unserialize(stream);
		metaData.SetAntenna2(ant);
	}
	if(hasBand)
	{
		BandInfo band;
		band.Unserialize(stream);
		metaData.SetBand(band);
	}
	if(hasObsTimes)
	{
		std::vector<double> vals(Serializable::UnserializeUInt64(stream));
		for(double& t : vals)
			t = Serializable::UnserializeDouble(stream);
		metaData.SetObservationTimes(vals);
	}
	return metaData;
}

Image2D SingleBaselineFile::UnserializeImage(std::istream& stream)
{
	size_t
		width = Serializable::UnserializeUInt64(stream),
		height = Serializable::UnserializeUInt64(stream);
	Image2D result = Image2D::MakeUnsetImage(width, height);
	for(size_t y=0; y!=height; ++y)
	{
		for(size_t x=0; x!=width; ++x)
		{
			result.SetValue(x, y, Serializable::UnserializeFloat(stream));
		}
	}
	return result;
}

Mask2D SingleBaselineFile::UnserializeMask(std::istream& stream)
{
	size_t
		width = Serializable::UnserializeUInt64(stream),
		height = Serializable::UnserializeUInt64(stream);
	Mask2D result = Mask2D::MakeUnsetMask(width, height);
	for(size_t y=0; y!=height; ++y)
	{
		for(size_t x=0; x!=width; ++x)
		{
			char val;
			stream.read(&val, 1);
			result.SetValue(x, y, val!=0);
		}
	}
	return result;
}

void SingleBaselineFile::Serialize(std::ostream& stream, const TimeFrequencyData& data)
{
	Serializable::SerializeToUInt32(stream, data.PolarizationCount());
	int complCode;
	switch(data.ComplexRepresentation())
	{
		default:
		case TimeFrequencyData::PhasePart: complCode = 0; break;
		case TimeFrequencyData::AmplitudePart: complCode = 1; break;
		case TimeFrequencyData::RealPart: complCode = 2; break;
		case TimeFrequencyData::ImaginaryPart: complCode = 3; break;
		case TimeFrequencyData::ComplexParts: complCode = 4; break;
	}
	Serializable::SerializeToUInt32(stream, complCode);
	for(size_t i=0; i!=data.PolarizationCount(); ++i)
	{
		PolarizationEnum p = data.GetPolarization(i);
		Serializable::SerializeToUInt32(stream, Polarization::EnumToAipsIndex(p));
		unsigned int imageFlagBitset = 0;
		TimeFrequencyData polData = data.MakeFromPolarizationIndex(i);
		if(polData.ImageCount() == 2)
			imageFlagBitset = imageFlagBitset | 0x02;
		if(polData.ImageCount() == 1)
			imageFlagBitset = imageFlagBitset | 0x01;
		if(polData.MaskCount() == 1)
			imageFlagBitset = imageFlagBitset | 0x04;
		Serializable::SerializeToUInt32(stream, imageFlagBitset);
		if(polData.ImageCount() >= 1)
			Serialize(stream, *polData.GetImage(0));
		if(polData.ImageCount() == 2)
			Serialize(stream, *polData.GetImage(1));
		if(polData.MaskCount() == 1)
			Serialize(stream, *polData.GetMask(0));
	}
}

void SingleBaselineFile::Serialize(std::ostream& stream, const TimeFrequencyMetaData& metaData)
{
	size_t featureSet = 0;
	if(metaData.HasAntenna1())
		featureSet = featureSet | 0x01;
	if(metaData.HasAntenna2())
		featureSet = featureSet | 0x02;
	if(metaData.HasBand())
		featureSet = featureSet | 0x04;
	//if(metaData.HasField())
	//	featureSet = featureSet | 0x08;
	if(metaData.HasObservationTimes())
		featureSet = featureSet | 0x10;
	//if(metaData.HasUVW())
	//	featureSet = featureSet | 0x20;
	Serializable::SerializeToUInt64(stream, featureSet);
	if(metaData.HasAntenna1())
		metaData.Antenna1().Serialize(stream);
	if(metaData.HasAntenna2())
		metaData.Antenna2().Serialize(stream);
	if(metaData.HasBand())
		metaData.Band().Serialize(stream);
	if(metaData.HasObservationTimes())
	{
		const std::vector<double>& vals = metaData.ObservationTimes();
		Serializable::SerializeToUInt64(stream, vals.size());
		for(const double& t : vals)
			Serializable::SerializeToDouble(stream, t);
	}
}

void SingleBaselineFile::Serialize(std::ostream& stream, const Image2D& image)
{
	Serializable::SerializeToUInt64(stream, image.Width());
	Serializable::SerializeToUInt64(stream, image.Height());
	for(size_t y=0; y!=image.Height(); ++y)
	{
		for(size_t x=0; x!=image.Width(); ++x)
		{
			Serializable::SerializeToFloat(stream, image.Value(x, y));
		}
	}
}

void SingleBaselineFile::Serialize(std::ostream& stream, const Mask2D& mask)
{
	Serializable::SerializeToUInt64(stream, mask.Width());
	Serializable::SerializeToUInt64(stream, mask.Height());
	for(size_t y=0; y!=mask.Height(); ++y)
	{
		for(size_t x=0; x!=mask.Width(); ++x)
		{
			char val = mask.Value(x, y) ? 1 : 0;
			stream.write(&val, 1);
		}
	}
}
