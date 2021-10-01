/*
 * UARTLogHandler.h
 *
 *  Created on: Jul 30, 2021
 *      Author: reedt
 */

#ifndef SRC_SHARED_LOGGING_UARTLOGHANDLER_H_
#define SRC_SHARED_LOGGING_UARTLOGHANDLER_H_

#include "Logger.h"
#include "../UARTManager.h"

class UARTLogHandler
{
private:
	UARTManager *uart;
	LogLevel _level;

	const char* extractFileName(const char *s)
	{
		const char *s1 = strrchr(s, '/');
		if (s1)
		{
			return s1 + 1;
		}
		return s;
	}

	const char* extractFuncName(const char *s, size_t *size)
	{
		const char *s1 = s;
		for (; *s; ++s)
		{
			if (*s == ' ')
			{
				s1 = s + 1; // Skip return type
			}
			else if (*s == '(')
			{
				break; // Skip argument types
			}
		}
		*size = s - s1;
		return s1;
	}

	explicit UARTLogHandler(){};

	static inline UARTLogHandler* getInstance()
	{
		static UARTLogHandler instance;
		return &instance; //UARTLogHandler.instance;
	}

	static void log_message_callback(const char *msg, int level,
			const char *category, const LogAttributes *attr, void *reserved)
	{
		UARTLogHandler *handler = getInstance();
		handler->message(msg, (LogLevel) level, category, *attr);

	}

	static void log_write_callback(const char *data, size_t size, int level,
			const char *category, void *reserved)
	{
		UARTLogHandler *handler = getInstance();

		handler->write(data, size, (LogLevel) level, category);

	}

	static int log_enabled_callback(int level, const char *category,
			void *reserved)
	{
		UARTLogHandler *handler = getInstance();
		return level >= (uint8_t) handler->level();
	}

public:
	/*!
	 \brief Constructor.
	 \param stream Output stream.
	 \param level Default logging level.
	 \param filters Category filters.
	 */
//	explicit UARTLogHandler(UARTManager *uart, LogLevel level = LOG_LEVEL_INFO) :
//			uart(uart), _level(level)
//	{
//		instance = this;
//		log_set_callbacks(log_message_callback, log_write_callback,
//				log_enabled_callback, nullptr);
//	}
	static UARTLogHandler* configure(UARTManager *uart, LogLevel level =
			LOG_LEVEL_INFO)
	{
		UARTLogHandler *instance(getInstance());
		instance->uart = uart;
		instance->_level = level;
		log_set_callbacks(log_message_callback, log_write_callback,
				log_enabled_callback, nullptr);
		return instance; //UARTLogHandler.instance;
	}

	void write(const char *data, size_t size, LogLevel level,
			const char *category)
	{
		if (level >= _level)
		{
			write(data, size);
		}
	}

	/*!
	 \brief Returns level name.
	 \param level Logging level.
	 */
	static const char* levelName(LogLevel level)
	{
		return log_level_name(level, nullptr);
	}

	/*!
	 \brief Returns default logging level.
	 */
	LogLevel level()
	{
		return _level;
	}

	// These methods are called by the LogManager
	void message(const char *msg, LogLevel level, const char *category,
			const LogAttributes &attr)
	{
		if (level >= _level)
		{
			logMessage(msg, level, category, attr);
		}
	}

	UARTLogHandler(const UARTLogHandler&) = delete;
	UARTLogHandler& operator=(const UARTLogHandler&) = delete;

protected:
	/*!
	 \brief Formats log message and writes it to output stream.
	 \param msg Text message.
	 \param level Logging level.
	 \param category Category name (can be null).
	 \param attr Message attributes.

	 Default implementation generates messages in the following format:
	 `<timestamp> [category] [file]:[line], [function]: <level>: <message> [attributes]`.
	 */
	void logMessage(const char *msg, LogLevel level, const char *category,
			const LogAttributes &attr)
	{
		const char *s = nullptr;
		// Timestamp
		if (attr.has_time)
		{
			printf("%010u ", (unsigned) attr.time);
		}
		// Category
		if (category)
		{
			write('[');
			write(category);
			write("] ", 2);
		}
		// Source file
		if (attr.has_file)
		{
			s = extractFileName(attr.file); // Strip directory path
			write(s); // File name
			if (attr.has_line)
			{
				write(':');
				printf("%d", (int) attr.line); // Line number
			}
			if (attr.has_function)
			{
				write(", ", 2);
			}
			else
			{
				write(": ", 2);
			}
		}
		// Function name
		if (attr.has_function)
		{
			size_t n = 0;
			s = extractFuncName(attr.function, &n); // Strip argument and return types
			write(s, n);
			write("(): ", 4);
		}
		// Level
		s = levelName(level);
		write(s);
		write(": ", 2);
		// Message
		if (msg)
		{
			write(msg);
		}
		// Additional attributes
		if (attr.has_code || attr.has_details)
		{
			write(" [", 2);
			// Code
			if (attr.has_code)
			{
				write("code = ", 7);
				printf("%" PRIiPTR, (intptr_t) attr.code);
			}
			// Details
			if (attr.has_details)
			{
				if (attr.has_code)
				{
					write(", ", 2);
				}
				write("details = ", 10);
				write(attr.details);
			}
			write(']');
		}
		write("\r\n", 2);
	}

	/*!
	 \brief Writes character buffer to output stream.
	 \param data Buffer.
	 \param size Buffer size.

	 This method is equivalent to `stream()->write((const uint8_t*)data, size)`.
	 */
	inline void write(const char *data, size_t size)
	{
		char buf[size + 1];
		memcpy(buf, data, size);
		buf[size] = '\0';
		uart->print(buf);
	}
	/*!
	 \brief Writes string to output stream.
	 \param str String.

	 This method is equivalent to `write(str, strlen(str))`.
	 */
	inline void write(const char *str)
	{
		write(str, strlen(str));
	}
	/*!
	 \brief Writes character to output stream.
	 \param c Character.
	 */
	inline void write(char c)
	{
		write(&c, 1);
	}
	/*!
	 \brief Formats string and writes it to output stream.
	 \param fmt Format string.

	 This method is equivalent to `stream()->printf(fmt, ...)`.
	 */
	inline void printf(const char *fmt, ...)
			__attribute__((format(printf, 2, 3)))
	{
		va_list args;
		va_start(args, fmt);
		uart->vprint(fmt, args);
		va_end(args);
	}
};

#endif /* SRC_SHARED_LOGGING_UARTLOGHANDLER_H_ */
