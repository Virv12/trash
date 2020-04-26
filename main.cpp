#include <string.h>
#include <assert.h>

#include <filesystem>
#include <algorithm>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
using namespace std;

size_t const maxSize = 1 << 29; // 512 MiB
const char* const trash = "/home/filippo/.cache/trash";
const char* const units = "B\0\0\0KiB\0MiB\0GiB\0TiB\0PiB\0EiB\0ZiB\0YiB";
size_t const trashLen = strlen(trash);

void toTrash(const char* const file) {
	if (access(file, R_OK) == -1) {
		fprintf(stderr, "Can not read file: %s\n", file);
		return;
	}

	char rp[PATH_MAX];
	realpath(file, rp);

	time_t t = time(0);
	size_t S = snprintf(nullptr, 0, "%s/%s/%ld.zip", trash, rp, t);
	char full_path[S + 1];
	sprintf(full_path, "%s/%s/%ld.zip", trash, rp, t);

	for (size_t i = trashLen + 1; full_path[i]; i++)
		if (full_path[i] == '/')
			full_path[i] = '%';

	pid_t pid = fork();
	if (pid) {
		int status;
		if (waitpid(pid, &status, 0) == -1) {
			fprintf(stderr, "Error zipping: %s\n", file);
			return;
		}
		if (WIFEXITED(status) && WEXITSTATUS(status)) {
			fprintf(stderr, "Error compressing, return code: %d\n", WEXITSTATUS(status));
			return;
		}
	} else {
		execlp("zip", "zip", "-rq", full_path, rp, nullptr);
		exit(1);
	}

	filesystem::remove_all(file);
}

void clean() {
	size_t T = 0;
	vector<tuple<time_t, size_t, const char*>> V;
	DIR *dir;
	struct stat stat_path, stat_entry;
	dirent *entry;

	stat(trash, &stat_path);
	if (!(dir = opendir(trash))) {
		fprintf(stderr, "Can't open directory: %s\n", trash);
		exit(1);
	}

	while ((entry = readdir(dir))) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;

		char* full_path = new char[trashLen + strlen(entry->d_name) + 2];
		sprintf(full_path, "%s/%s", trash, entry->d_name);

		stat(full_path, &stat_entry);
		V.emplace_back(-stat_entry.st_ctime, stat_entry.st_size, full_path); // TODO: get disk size
	}

	closedir(dir);

	sort(V.begin(), V.end());
	for (auto [ctime, S, file] : V) {
		if (T + S > maxSize) {
			printf("Removing %s\n", file);
			remove(file);
		} else {
			T += S;
		}

		delete[] file;
	}

	size_t unit = 0;
	while (T > 16384) {
		T /= 1024;
		unit++;
	}

	printf("Trash size: %lu%s\n", T, &units[4 * unit]);
}

int main(int argc, char** argv) {
	(void)argc;

	filesystem::create_directories(trash);

	bool escape = false;
	for (int i = 1; i < argc; i++)
		if (escape) {
			toTrash(argv[i]);

		} else if (!strcmp(argv[i], "--")) {
			escape = true;

		} else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			printf("Usage: %s [-h | --help] [--] [FILE]...\n", argv[0]);
			exit(0);

		} else {
			toTrash(argv[i]);
		}

	clean();
}
