#include <stdarg.h>
#include <stdio.h>

#include <esphome/components/logger.h>

void esphome_logger_log(const char *level, const char *tag, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	printf("\n");
	va_end(args);
}
