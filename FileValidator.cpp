#include "FileValidator.h"
#include <iostream>

using namespace std;

void FileValidator::validateSyntax(const string& filePath) {
    cout << "Validating syntax for: " << filePath << endl;
}

void FileValidator::detectErrors() {
    cout << "Detecting errors..." << endl;
}

void FileValidator::informUser() {
    cout << "Informing user about file issues..." << endl;
}
