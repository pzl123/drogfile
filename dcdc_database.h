#ifndef DCDC_DATABASE_H
#define DCDC_DATABASE_H

#include <iostream>
#include <sqlite3.h>


sqlite3 *open_db(const std::string& db_path);

void close_db(sqlite3 *db);

void drop_table_in_db(string& table_name, sqlite3 *db);

void select_between_id(void *usr_data, int id1, int id2, std::string table_name, sqlite3 *db);

void select_at_timestamp(void *usr_data, std::string timestamp, std::string table_name, sqlite3 *db);

void select_between_timestamp(void *usr_data, std::string timestamp1, std::string timestamp2, std::string table_name, sqlite3 *db);

void handle_can_log(std::string &log_path);

#endif
