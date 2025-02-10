#ifndef FILEVALIDATOR_H
#define FILEVALIDATOR_H

#include <string>

using namespace std;

class FileValidator {
public:
    void validateSyntax(const string& filePath);
    void detectErrors();
    void informUser();
};

#endif // FILEVALIDATOR_H
