#include "fs_watcher.h"

#include <sys/inotify.h>
#include <unistd.h>
#include <filesystem>
#include <chrono>

#define INOTIFY_BUF_LEN 4096

namespace fs = std::filesystem;
using namespace std::chrono;

static int inotify_fd = -1;
static steady_clock::time_point last_change {};
static constexpr auto DEBOUNCE = milliseconds(1000);

void fsWatcherInit(const char* sdPath)
{
  if (!sdPath || !sdPath[0]) return;

  inotify_fd = inotify_init1(IN_NONBLOCK);
  if (inotify_fd < 0) return;

  try {
    if (!fs::is_directory(sdPath)) return;

    uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY |
                    IN_MOVED_FROM | IN_MOVED_TO;

    inotify_add_watch(inotify_fd, sdPath, mask);

    for (auto& entry :
         fs::recursive_directory_iterator(sdPath,
                                          fs::directory_options::skip_permission_denied)) {
      if (entry.is_directory()) {
        inotify_add_watch(inotify_fd, entry.path().c_str(), mask);
      }
    }
  } catch (...) {
  }
}

bool fsWatcherCheck()
{
  if (inotify_fd < 0) return false;

  auto now = steady_clock::now();
  if (now - last_change < DEBOUNCE) return false;

  char buf[INOTIFY_BUF_LEN]
      __attribute__((aligned(__alignof__(struct inotify_event))));
  bool changed = false;

  ssize_t len;
  while ((len = read(inotify_fd, buf, sizeof(buf))) > 0) {
    char* ptr = buf;
    while (ptr < buf + len) {
      auto* event = (struct inotify_event*)ptr;
      if (event->mask & IN_Q_OVERFLOW) {
        changed = true;
      } else if (event->len > 0 && event->name[0] != '.') {
        changed = true;
      }
      ptr += sizeof(struct inotify_event) + event->len;
    }
  }

  if (changed) last_change = now;
  return changed;
}
