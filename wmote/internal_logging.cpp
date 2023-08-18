#include "internal_logging.hpp"



void log_info(std::string const &message) {
    if (s_logger)
        s_logger(message);
}

void log_error(std::string const &message) {
    if (s_error_logger)
        s_error_logger(message);
}