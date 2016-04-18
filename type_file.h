#ifndef TYPE_FILE
#define TYPE_FILE

#include "comman.h"

class TypeFile {

private:
	char folderName[100];
	DIR *directory[2];
	dirent *file[2];

public:

	TypeFile(char *_folderName) {
		sprintf(folderName, "test/%s/", _folderName);
		for (int i = 0; i < 2; i++) {
			directory[i] = NULL;
			file[i] = NULL;
		}
	}

	char* getNextFileName(const int folderTag) { // 0 : input | 1 : groundtruth

		switch (folderTag) {
		case 0:
			strcat(folderName, "input/");
			break;
		case 1:
			strcat(folderName, "groundtruth/");
			break;
		default:
			cout << "error in getNextFileName";
			return NULL;
		}

		if (directory[folderTag] == NULL) {
			directory[folderTag] = opendir(folderName);
		}

		while ((file[folderTag] = readdir(directory[folderTag])) != NULL) {
			if (strcmp(file[folderTag]->d_name, ".") != 0 && strcmp(file[folderTag]->d_name, "..") != 0) break;
		}

		if (file[folderTag] == NULL) {
			return NULL;
		} else {
			char *fileName = folderName;
			strcat(fileName, file[folderTag]->d_name);
			return fileName;
		}
	}
};

#endif // TYPE_FILE

