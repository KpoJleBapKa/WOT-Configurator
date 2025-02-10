#include "main.h"
#include <iostream>

using namespace std;

void ChangeTracker::logChanges() {
    cout << "Logging changes..." << endl;
}

void ChangeTracker::rollbackChanges() {
    cout << "Rolling back changes..." << endl;
}

void ChangeTracker::displayChangeHistory() {
    cout << "Displaying change history..." << endl;
}
