#ifndef DCDC_DATABASE_H
#define DCDC_DATABASE_H

#include <iostream>
#include <sqlite3.h>


sqlite3 *open_db(const std::string& db_path);

#endif
