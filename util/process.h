#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

#include <functional>

class Process {
 public:
  Process(const std::string& cmdLine) {
    _pid = vfork();
    switch (_pid) {
      case -1:  // Error
        throw std::runtime_error(
            "Could not vfork() new process for executing remote client");
      case 0:  // Child
        execl("/bin/sh", "sh", "-c", cmdLine.c_str(), NULL);
        _exit(127);
    }
  }

  void Join() {
    // Wait for process to terminate
    int pStatus;
    do {
      int pidReturn;
      do {
        pidReturn = waitpid(_pid, &pStatus, 0);
      } while (pidReturn == -1 && errno == EINTR);
    } while (!WIFEXITED(pStatus) && !WIFSIGNALED(pStatus));
    if (WIFEXITED(pStatus)) {
      const int exitStatus = WEXITSTATUS(pStatus);
      onFinished(exitStatus != 0, exitStatus);
    } else {
      onFinished(true, 0);
    }
  }

 private:
  std::function<void(bool, int)> onFinished;  // TODO

  int _pid;
};
