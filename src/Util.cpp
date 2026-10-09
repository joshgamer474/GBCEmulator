#include <Util.h>

#include <filesystem>
#include <cstdlib>
#include <stdexcept>

std::string get_log_directory()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
   return "";
#elif __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
        const char* home_dir = std::getenv("HOME");
        if (!home_dir)
        {
            throw std::runtime_error("Missing iOS app home directory for logs");
        }
        return std::string(home_dir) + "/Library/Caches/GBCEmulator/Logs/";
    #elif TARGET_OS_IPHONE
        const char* home_dir = std::getenv("HOME");
        if (!home_dir) throw std::runtime_error("Missing iOS app home directory for logs");
        {
            return std::string(home_dir) + "/Library/Caches/GBCEmulator/Logs/";
        }
    #elif TARGET_OS_MAC
        const char* home_dir = std::getenv("HOME");
        return std::string(home_dir) + "/.gbcemulator/";
    #else
    #   error "Unknown Apple platform"
    #endif
#elif __ANDROID__
    return "/usr/var/log/";
#elif __linux__
    return "~/.cache/log/";
#elif __unix__ // all unices not caught above
    return "~/.cache/log/";
#elif defined(_POSIX_VERSION)
    // POSIX
#else
#   error "Unknown compiler"
#endif
  return "AAAA";
}

std::string generate_log_path(const std::string& rom_file)
{
  // Get log directory based on OS
  const std::string log_dir = get_log_directory();
  if (log_dir.empty())
  {
    // Default log directory is same as ROM file
    return rom_file + ".log";
  }

  // Get filename from rom_file
  const std::filesystem::path p(rom_file);
  const std::string filename = p.filename().string();
  return log_dir + filename + ".log";
}
