/** @file
 * This is the header file for some of the exception classes and for the base exception class BaseException.
 * @author André Offringa <offringa@gmail.com>
 */
#ifndef BASEEXCEPTION_H
#define BASEEXCEPTION_H

#include <stdexcept>
#include <string>

/**
 * The base exception class that all exceptions should inherit from.
 */
class BaseException : public std::runtime_error {
	public:
		/**
		 * Constructor that initialises the exception as if an unspecified error occured.
		 */
		BaseException() noexcept : std::runtime_error("Unspecified error") { }
		
		/**
		 * Constructor that initialises the exception with a specified description.
		 * @param description The description that should describe the cause of the exception.
		 */
		explicit BaseException(const std::string &description) noexcept : std::runtime_error(description) { }
		
		/**
		 * Destructor.
		 */
		virtual ~BaseException() noexcept { }
		
	private:
};

/**
 * Exception that is throwed in case of Input/Output exceptions.
 */
class IOException : public BaseException {
	public:
		/**
		 * Constructor that initialises the IOException without a description.
		 */
		IOException() noexcept : BaseException() { }
		
		/**
		 * Constructor that initialises the IOException with a description
		 * @param description The description of the Input/Output exception
		 */
		explicit IOException(const std::string &description) noexcept : BaseException(description) { }
};

/**
 * Exception that is throwed in case of an incorrect configuration
 */
class ConfigurationException : public BaseException {
	public:
		/**
		 * Constructor that initialises the ConfigurationException with a description
		 * @param description The description of the configuration error
		 */
		explicit ConfigurationException(const std::string &description) noexcept : BaseException(description) { }
};

/**
 * Exception that is throwed in case of an incorrect usage of a function
 */
class BadUsageException : public BaseException {
	public:
		/**
		 * Constructor that initialises the BadUsageException with a description
		 * @param description The description of the incorrect usage
		 */
		explicit BadUsageException(const std::string &description) noexcept : BaseException(description) { }
};
#endif
