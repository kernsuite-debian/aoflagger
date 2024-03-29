#include "fitsfile.h"

#include <limits>
#include <cmath>

#include <sstream>
#include <iostream>

#include <boost/algorithm/string/trim.hpp>

#include "../util/logger.h"

FitsFile::FitsFile(const std::string& filename)
    : _filename(filename), _fptr(nullptr), _isOpen(false) {}

FitsFile::~FitsFile() {
  if (_isOpen) Close();
}

void FitsFile::CheckStatus(int status) const {
  if (status) {
    /* fits_get_errstatus returns at most 30 characters */
    char err_text[31];
    fits_get_errstatus(status, err_text);
    char err_msg[81];
    std::stringstream errMsg;
    errMsg << "CFITSIO reported error when performing IO on file '" << _filename
           << "':" << err_text << " (";
    while (fits_read_errmsg(err_msg)) errMsg << err_msg;
    errMsg << ')';
    throw FitsIOException(errMsg.str());
  }
}

void FitsFile::CheckOpen() const {
  if (!_isOpen) throw FitsIOException("No open file, call Open() first");
}

void FitsFile::Open(FitsFile::FileMode mode) {
  if (_isOpen) {
    throw FitsIOException("File was opened twice");
  } else {
    int status = 0;
    int modeInt = 0;
    switch (mode) {
      case ReadOnlyMode:
        modeInt = READONLY;
        break;
      case ReadWriteMode:
        modeInt = READWRITE;
        break;
      default:
        throw FitsIOException("Incorrect mode specified");
        break;
    }
    fits_open_diskfile(&_fptr, _filename.c_str(), modeInt, &status);
    CheckStatus(status);
    _isOpen = true;
  }
}

void FitsFile::Create() {
  if (_isOpen) {
    throw FitsIOException("File was opened twice");
  } else {
    int status = 0;
    fits_create_file(&_fptr, (std::string("!") + _filename).c_str(), &status);
    CheckStatus(status);
    _isOpen = true;
  }
}

void FitsFile::Close() {
  if (_isOpen) {
    int status = 0;
    fits_close_file(_fptr, &status);
    CheckStatus(status);
    _isOpen = false;
    _fptr = nullptr;
  } else {
    throw FitsIOException("Non-opened file was closed");
  }
}

int FitsFile::GetHDUCount() {
  CheckOpen();
  int hdunum = 0, status = 0;
  fits_get_num_hdus(_fptr, &hdunum, &status);
  CheckStatus(status);
  return hdunum;
}

int FitsFile::GetCurrentHDU() {
  CheckOpen();
  int hdunum = 0;
  fits_get_hdu_num(_fptr, &hdunum);
  return hdunum;
}

void FitsFile::MoveToHDU(int hduNumber) {
  CheckOpen();
  int status = 0;
  fits_movabs_hdu(_fptr, hduNumber, nullptr, &status);
  CheckStatus(status);
}

enum FitsFile::HDUType FitsFile::GetCurrentHDUType() {
  CheckOpen();
  int hdutypeInt = 0, status = 0;
  fits_get_hdu_type(_fptr, &hdutypeInt, &status);
  CheckStatus(status);
  enum HDUType hduType;
  switch (hdutypeInt) {
    case IMAGE_HDU:
      hduType = ImageHDUType;
      break;
    case ASCII_TBL:
      hduType = ASCIITableHDUType;
      break;
    case BINARY_TBL:
      hduType = BinaryTableHDUType;
      break;
    default:
      throw FitsIOException("Unknown HDUType returned");
  }
  return hduType;
}

enum FitsFile::ImageType FitsFile::GetCurrentImageType() {
  CheckOpen();
  int bitPixInt = 0, status = 0;
  fits_get_img_type(_fptr, &bitPixInt, &status);
  CheckStatus(status);
  enum ImageType imageType;
  switch (bitPixInt) {
    case BYTE_IMG:
      imageType = Int8ImageType;
      break;
    case SHORT_IMG:
      imageType = Int16ImageType;
      break;
    case LONG_IMG:
      imageType = Int32ImageType;
      break;
    case FLOAT_IMG:
      imageType = Float32ImageType;
      break;
    case DOUBLE_IMG:
      imageType = Double64ImageType;
      break;
    default:
      throw FitsIOException("Unknown image type returned");
  }
  return imageType;
}

int FitsFile::GetCurrentImageDimensionCount() {
  CheckOpen();
  int status = 0, naxis = 0;
  fits_get_img_dim(_fptr, &naxis, &status);
  CheckStatus(status);
  return naxis;
}

long FitsFile::GetCurrentImageSize(int dimension) {
  CheckOpen();
  if (dimension > GetCurrentImageDimensionCount())
    throw FitsIOException("Parameter outside range");
  int status = 0;
  long* sizes = new long[dimension];
  fits_get_img_size(_fptr, dimension, sizes, &status);
  const long size = sizes[dimension - 1];
  delete[] sizes;
  CheckStatus(status);
  return size;
}

void FitsFile::ReadCurrentImageData(long startPos, num_t* buffer,
                                    long bufferSize, long double nullValue) {
  CheckOpen();
  int status = 0, dimensions = GetCurrentImageDimensionCount();
  long* firstpixel = new long[dimensions];
  for (int i = 0; i < dimensions; i++) {
    firstpixel[i] = 1 + startPos % GetCurrentImageSize(i + 1);
    startPos = startPos / GetCurrentImageSize(i + 1);
  }
  double* dblbuffer = new double[bufferSize];
  double dblNullValue = nullValue;
  int anynul = 0;
  fits_read_pix(_fptr, TDOUBLE, firstpixel, bufferSize, &dblNullValue,
                dblbuffer, &anynul, &status);
  for (int i = 0; i < bufferSize; i++) buffer[i] = dblbuffer[i];
  delete[] dblbuffer;
  delete[] firstpixel;
  CheckStatus(status);
}

void FitsFile::AppendImageHUD(enum FitsFile::ImageType imageType, long width,
                              long height) {
  int status = 0;
  int bitPixInt;
  switch (imageType) {
    case Int8ImageType:
      bitPixInt = BYTE_IMG;
      break;
    case Int16ImageType:
      bitPixInt = SHORT_IMG;
      break;
    case Int32ImageType:
      bitPixInt = LONG_IMG;
      break;
    case Float32ImageType:
      bitPixInt = FLOAT_IMG;
      break;
    case Double64ImageType:
      bitPixInt = DOUBLE_IMG;
      break;
    default:
      throw FitsIOException("?");
  }
  long* naxes = new long[2];
  naxes[0] = width;
  naxes[1] = height;
  fits_create_img(_fptr, bitPixInt, 2, naxes, &status);
  delete[] naxes;
  CheckStatus(status);
}

void FitsFile::WriteImage(long startPos, double* buffer, long bufferSize,
                          double nullValue) {
  CheckOpen();
  int status = 0, dimensions = GetCurrentImageDimensionCount();
  long* firstpixel = new long[dimensions];
  for (int i = 0; i < dimensions; i++) {
    firstpixel[i] = 1 + startPos % GetCurrentImageSize(i + 1);
    startPos = startPos / GetCurrentImageSize(i + 1);
  }
  fits_write_pixnull(_fptr, TDOUBLE, firstpixel, bufferSize, buffer, &nullValue,
                     &status);
  delete[] firstpixel;
  CheckStatus(status);
}

void FitsFile::WriteImage(long startPos, float* buffer, long bufferSize,
                          float nullValue) {
  CheckOpen();
  int status = 0, dimensions = GetCurrentImageDimensionCount();
  long* firstpixel = new long[dimensions];
  for (int i = 0; i < dimensions; i++) {
    firstpixel[i] = 1 + startPos % GetCurrentImageSize(i + 1);
    startPos = startPos / GetCurrentImageSize(i + 1);
  }
  fits_write_pixnull(_fptr, TFLOAT, firstpixel, bufferSize, buffer, &nullValue,
                     &status);
  delete[] firstpixel;
  CheckStatus(status);
}

int FitsFile::GetRowCount() {
  CheckOpen();
  long rowCount;
  int status = 0;
  fits_get_num_rows(_fptr, &rowCount, &status);
  CheckStatus(status);
  return rowCount;
}

int FitsFile::GetKeywordCount() {
  int status = 0, keysexist;
  fits_get_hdrspace(_fptr, &keysexist, nullptr, &status);
  CheckStatus(status);
  return keysexist;
}

std::string FitsFile::GetKeyword(int keywordNumber) {
  char keyName[FLEN_KEYWORD], keyValue[FLEN_VALUE];
  int status = 0;
  fits_read_keyn(_fptr, keywordNumber, keyName, keyValue, nullptr, &status);
  CheckStatus(status);
  return std::string(keyName);
}

std::string FitsFile::GetKeywordValue(int keywordNumber) {
  char keyName[FLEN_KEYWORD], keyValue[FLEN_VALUE];
  int status = 0;
  fits_read_keyn(_fptr, keywordNumber, keyName, keyValue, nullptr, &status);
  CheckStatus(status);
  std::string val(keyValue);
  if (val.length() >= 2 && *val.begin() == '\'' && *val.rbegin() == '\'') {
    val = val.substr(1, val.length() - 2);
    boost::trim(val);
  }
  return val;
}

bool FitsFile::GetKeywordValue(const std::string& keywordName,
                               std::string& value) {
  char keyValue[FLEN_VALUE];
  int status = 0;
  fits_read_keyword(_fptr, const_cast<char*>(keywordName.c_str()), keyValue,
                    nullptr, &status);
  if (status == 0) {
    value = std::string(keyValue);
    if (value.length() >= 2 && *value.begin() == '\'' &&
        *value.rbegin() == '\'') {
      value = value.substr(1, value.length() - 2);
      boost::trim(value);
    }
    return true;
  } else {
    return false;
  }
}

std::string FitsFile::GetKeywordValue(const std::string& keywordName) {
  char keyValue[FLEN_VALUE];
  int status = 0;
  fits_read_keyword(_fptr, const_cast<char*>(keywordName.c_str()), keyValue,
                    nullptr, &status);
  CheckStatus(status);
  std::string val(keyValue);
  if (val.length() >= 2 && *val.begin() == '\'' && *val.rbegin() == '\'') {
    val = val.substr(1, val.length() - 2);
    boost::trim(val);
  }
  return val;
}

std::string FitsFile::GetKeywordComment(int keywordNumber) {
  char keyName[FLEN_KEYWORD], keyValue[FLEN_VALUE], keyComment[FLEN_COMMENT];
  int status = 0;
  fits_read_keyn(_fptr, keywordNumber, keyName, keyValue, keyComment, &status);
  CheckStatus(status);
  return std::string(keyComment);
}

int FitsFile::GetColumnCount() {
  CheckOpen();
  int rowCount, status = 0;
  fits_get_num_cols(_fptr, &rowCount, &status);
  CheckStatus(status);
  return rowCount;
}

int FitsFile::GetColumnType(int colNumber) {
  CheckOpen();
  int typecode, status = 0;
  long repeat, width;
  fits_get_coltype(_fptr, colNumber, &typecode, &repeat, &width, &status);
  CheckStatus(status);
  return typecode;
}

int FitsFile::GetIntKeywordValue(int keywordNumber) {
  return atoi(GetKeyword(keywordNumber).c_str());
}

int FitsFile::GetIntKeywordValue(const std::string& keywordName) {
  return atoi(GetKeywordValue(keywordName).c_str());
}

double FitsFile::GetDoubleKeywordValue(int keywordNumber) {
  return atof(GetKeyword(keywordNumber).c_str());
}

double FitsFile::GetDoubleKeywordValue(const std::string& keywordName) {
  return atof(GetKeywordValue(keywordName).c_str());
}

bool FitsFile::HasGroups() {
  try {
    return GetKeywordValue("GROUPS") == "T";
  } catch (FitsIOException& e) {
    return false;
  }
}

int FitsFile::GetGroupCount() { return GetIntKeywordValue("GCOUNT"); }

int FitsFile::GetParameterCount() { return GetIntKeywordValue("PCOUNT"); }

long FitsFile::GetImageSize() {
  long size = 1;
  for (int i = 2; i <= GetCurrentImageDimensionCount(); ++i) {
    size *= GetCurrentImageSize(i);
  }
  return size;
}

long FitsFile::GetGroupSize() {
  if (!HasGroups()) throw FitsIOException("HDU has no groups");
  long size = 1;
  for (int i = 2; i <= GetCurrentImageDimensionCount(); ++i) {
    size *= GetCurrentImageSize(i);
  }
  size += GetParameterCount();
  return size;
}

void FitsFile::ReadGroupParameters(long groupIndex,
                                   long double* parametersData) {
  int status = 0;
  const long pSize = GetParameterCount();
  double* parameters = new double[pSize];

  fits_read_grppar_dbl(_fptr, groupIndex + 1, 1, pSize, parameters, &status);
  CheckStatus(status);

  for (long i = 0; i < pSize; ++i) parametersData[i] = parameters[i];

  delete[] parameters;
}

void FitsFile::ReadGroup(long groupIndex, long double* groupData) {
  int status = 0;
  const long size = GetImageSize();
  const long pSize = GetParameterCount();
  double* parameters = new double[pSize];
  const double nulValue = std::numeric_limits<double>::quiet_NaN();
  int anynul = 0;

  fits_read_grppar_dbl(_fptr, groupIndex + 1, 1, pSize, parameters, &status);
  CheckStatus(status);

  for (long i = 0; i < pSize; ++i) groupData[i] = parameters[i];

  delete[] parameters;
  double* data = new double[size];

  fits_read_img_dbl(_fptr, groupIndex + 1, 1, size, nulValue, data, &anynul,
                    &status);
  CheckStatus(status);

  for (long i = 0; i < size; ++i) groupData[pSize + i] = data[i];

  delete[] data;

  if (anynul != 0) Logger::Warn << "There were nulls in the group\n";
}

void FitsFile::ReadGroupData(long groupIndex, long double* groupData) {
  int status = 0;
  const long size = GetImageSize();
  const double nulValue = std::numeric_limits<double>::quiet_NaN();
  int anynul = 0;

  double* data = new double[size];

  fits_read_img_dbl(_fptr, groupIndex + 1, 1, size, nulValue, data, &anynul,
                    &status);
  CheckStatus(status);

  for (long i = 0; i < size; ++i) groupData[i] = data[i];

  delete[] data;

  if (anynul != 0) Logger::Warn << "There were nulls in the group data\n";
}

int FitsFile::GetGroupParameterIndex(const std::string& parameterName) {
  if (!HasGroups()) throw FitsIOException("HDU has no groups");
  const int parameterCount = GetParameterCount();
  for (int i = 1; i <= parameterCount; ++i) {
    std::stringstream s;
    s << "PTYPE" << i;
    if (GetKeywordValue(s.str()) == parameterName) return i - 1;
  }
  throw FitsIOException(std::string("Can not find parameter with name ") +
                        parameterName);
}

int FitsFile::GetGroupParameterIndex(const std::string& parameterName,
                                     int number) {
  if (!HasGroups()) throw FitsIOException("HDU has no groups");
  const int parameterCount = GetParameterCount();
  for (int i = 1; i <= parameterCount; ++i) {
    std::stringstream s;
    s << "PTYPE" << i;
    if (GetKeywordValue(s.str()) == parameterName) {
      --number;
      if (number == 0) return i - 1;
    }
  }
  throw FitsIOException(std::string("Can not find parameter with name ") +
                        parameterName);
}

bool FitsFile::HasGroupParameter(const std::string& parameterName) {
  if (!HasGroups()) return false;
  const int parameterCount = GetParameterCount();
  for (int i = 1; i <= parameterCount; ++i) {
    std::stringstream s;
    s << "PTYPE" << i;
    if (GetKeywordValue(s.str()) == parameterName) return true;
  }
  return false;
}

bool FitsFile::HasGroupParameter(const std::string& parameterName, int number) {
  if (!HasGroups()) return false;
  const int parameterCount = GetParameterCount();
  for (int i = 1; i <= parameterCount; ++i) {
    std::stringstream s;
    s << "PTYPE" << i;
    if (GetKeywordValue(s.str()) == parameterName) {
      --number;
      if (number == 0) return true;
    }
  }
  return false;
}

bool FitsFile::HasTableColumn(const std::string& columnName, int& columnIndex) {
  const int colCount = GetColumnCount();
  for (int i = 1; i <= colCount; ++i) {
    std::stringstream s;
    s << "TTYPE" << i;
    if (GetKeywordValue(s.str()) == columnName) {
      columnIndex = i;
      return true;
    }
  }
  return false;
}

int FitsFile::GetTableColumnIndex(const std::string& columnName) {
  const int colCount = GetColumnCount();
  for (int i = 1; i <= colCount; ++i) {
    std::stringstream s;
    s << "TTYPE" << i;
    if (GetKeywordValue(s.str()) == columnName) return i;
  }
  throw FitsIOException(std::string("Can not find column with name ") +
                        columnName);
}

int FitsFile::GetTableColumnArraySize(int columnIndex) {
  CheckOpen();
  int typecode = 0, status = 0;
  long repeat = 0, width = 0;
  fits_get_coltype(_fptr, columnIndex, &typecode, &repeat, &width, &status);
  CheckStatus(status);
  return repeat;
}

std::vector<long> FitsFile::GetColumnDimensions(int columnIndex) {
  CheckOpen();
  int naxis = 0, status = 0;
  constexpr int maxdim = 10;
  std::vector<long> axes(maxdim, 0);
  fits_read_tdim(_fptr, columnIndex, maxdim, &naxis, axes.data(), &status);
  CheckStatus(status);
  axes.resize(naxis);
  return axes;
}

long FitsFile::GetColumnDimensionSize(int columnIndex, int dimension) {
  CheckOpen();
  int naxis = 0, status = 0, maxdim = 10;
  long naxes[10];
  for (size_t i = 0; i != 10; ++i) naxes[i] = 0;
  fits_read_tdim(_fptr, columnIndex, maxdim, &naxis, naxes, &status);
  CheckStatus(status);
  if (dimension >= naxis)
    throw FitsIOException(
        "Requested dimension index not available in fits file");
  return naxes[dimension];
}

std::string FitsFile::GetTableDimensionName(int index) {
  CheckOpen();
  std::ostringstream name;
  name << "CTYPE" << (index + 1);
  int status = 0;
  char valueStr[256], commentStr[256];
  fits_read_key(_fptr, TSTRING, name.str().c_str(), valueStr, commentStr,
                &status);
  std::string val;
  if (!status) {
    val = valueStr;
  }
  return val;
}

void FitsFile::ReadTableCell(int row, int col, double* output, size_t size) {
  int status = 0;
  double nulValue = std::numeric_limits<double>::quiet_NaN();
  int anynul = 0;
  fits_read_col(_fptr, TDOUBLE, col, row, 1, size, &nulValue, output, &anynul,
                &status);
}

void FitsFile::ReadTableCell(int row, int col, long double* output,
                             size_t size) {
  std::vector<double> data(size);
  int status = 0;
  double nulValue = std::numeric_limits<double>::quiet_NaN();
  int anynul = 0;
  fits_read_col(_fptr, TDOUBLE, col, row, 1, size, &nulValue, data.data(),
                &anynul, &status);
  for (size_t i = 0; i < size; ++i) output[i] = data[i];
}

void FitsFile::ReadTableCell(int row, int col, bool* output, size_t size) {
  std::vector<char> data(size);
  int status = 0;
  char nulValue = 0;
  int anynul = 0;
  fits_read_col(_fptr, TBIT, col, row, 1, size, &nulValue, data.data(), &anynul,
                &status);
  for (size_t i = 0; i < size; ++i) output[i] = data[i] != 0;
}

void FitsFile::ReadTableCell(int row, int col, char* output) {
  int status = 0;
  double nulValue = std::numeric_limits<double>::quiet_NaN();
  int anynul = 0;
  fits_read_col(_fptr, TSTRING, col, row, 1, 1, &nulValue, &output, &anynul,
                &status);
}

void FitsFile::WriteTableCell(int row, int col, double* data, size_t size) {
  int status = 0;
  fits_write_col(_fptr, TDOUBLE, col, row, 1, size, data, &status);
  CheckStatus(status);
}

void FitsFile::WriteTableCell(int row, int col, const bool* data, size_t size) {
  std::vector<char> dataChar(size);
  int status = 0;
  for (size_t i = 0; i < size; ++i) {
    dataChar[i] = data[i] ? 1 : 0;
  }
  fits_write_col(_fptr, TBIT, col, row, 1, size, dataChar.data(), &status);
}
