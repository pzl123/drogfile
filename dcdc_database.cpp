#include "dcdc_database.h"
#include <stdint.h>
#include <regex>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

#define MAX_LEN (8U)

typedef struct
{
    int id;
    std::string timestamp;
    unsigned int can_id;
    unsigned int can_len;
    unsigned int can_data[MAX_LEN];
} linestr_2_dbinfo_t;

void close_db(sqlite3 *db)
{
    if (SQLITE_OK != sqlite3_close(db))
    {
        std::cout << "close db error" << std::endl;
    }
}

sqlite3* open_db(const std::string& db_path)
{
    sqlite3 *db = NULL;
    int32_t ret = sqlite3_open(db_path.c_str(), &db);
    if (SQLITE_OK != ret)
    {
        std::cout << "open" << db_path << "error:" << sqlite3_errmsg(db) << std::endl;
        if (db)
        {
            sqlite3_close(db);
        }
        return NULL;
    }
    return db;
}

void exec_in_db(std::string &create_table_str, sqlite3* db)
{
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, create_table_str.c_str(), 0, 0, &err_msg);
    if (SQLITE_OK != rc)
    {
        std::cout << err_msg << std::endl;
        sqlite3_free(err_msg);
        sqlite3_close(db);
    }
}

void drop_table_in_db(string& table_name, sqlite3 *db)
{
    string delete_str = "DROP TABLE IF EXISTS ";
    string final_str = delete_str + table_name;
    exec_in_db(delete_str, db);
}

int32_t split_str_2_frame_data(std::string& str, unsigned int *data, unsigned int max_len)
{
    if (23 != str.size())
    {
        return -1;
    }
    std::regex re(R"(^([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})$)");
    std::smatch match;
    if (std::regex_match(str, match, re))
    {
        for (size_t i = 1; i <= 8; ++i)
        {
            data[i-1] = static_cast<unsigned int>(std::stoi(match[i].str(), nullptr, 16));
            // data[i-1] = static_cast<unsigned int>(data[i-1]);
            // std::cout << "data[" << (i-1) << "]: "<< std::hex << data[i-1] << " " << std::endl;
        }
    }
    else
    {
        std::cout << "error regex_match:" << str << std::endl;
    }
    return 0;
}

void string_info_2_dcdc_insert_msg(std::string& line_str, linestr_2_dbinfo_t &insert_msg)
{
    std::regex re(R"(\((\d{4}-\d{2}-\d{2})\s+(\d{2}:\d{2}:\d{2}\.\d+)\)\s+(\w+)\s+([0-9a-zA-Z]+)\s+\[(\d)\]\s+(([0-9a-zA-Z]){2}(?:\s+[0-9a-zA-Z]{2})*))");
    std::smatch match;
    if (std::regex_search(line_str, match, re))
    {
        // std::cout << "data:" << match[1] << std::endl;
        // std::cout << "time:" << match[2] << std::endl;
        // std::cout << "can_itf:" << match[3] << std::endl;
        // std::cout << "can_id:" << match[4] << std::endl;
        // std::cout << "can_len:" << match[5] << std::endl;
        // std::cout << "can_data:" << match[6] << std::endl;
    }
    else
    {
        std::cout << "regex error:" << line_str << std::endl;
        return;
    }
    insert_msg.can_id = static_cast<unsigned int>(std::stoi(match[4].str(), nullptr, 16));
    insert_msg.can_len =  static_cast<uint8_t>(std::stoi(match[5].str(), NULL, 10));
    insert_msg.timestamp  = match[1].str() + " " + match[2].str();
    string can_data_str = match[6].str();
    if (insert_msg.can_len <= MAX_LEN)
    {
        if (-1 == split_str_2_frame_data(can_data_str, insert_msg.can_data, MAX_LEN))
        {
            std::cout << "split str error: " << can_data_str << std::endl;
            return;
        }
    }
}

void print_insert_frame(linestr_2_dbinfo_t insert_msg)
{
    printf("ID: 0x%X, Data:", insert_msg.can_id);
    for (int i = 0; i < insert_msg.can_len; i++) {
        printf(" %02X", insert_msg.can_data[i]);
    }
    printf("\n");
}

static int select_msg_2_db_template(void *usr_data, int argc, char **argv, char **azColName)
{
    auto* result_vec = static_cast<std::vector<linestr_2_dbinfo_t>*>(usr_data);
    linestr_2_dbinfo_t insert_msg{};
    for (int i = 0; i < argc; i++)
    {
        std::string col_name = azColName[i];
        std::string value = argv[i] ? argv[i] : "";
        if ("" == value)
        {
            return -1;
        }

        if ("id" == col_name)
        {
            insert_msg.id = static_cast<int>(std::stoi(value));
        }
        else if ("timestamp")
        {
            insert_msg.timestamp = value;
        }
        else if ("can_id")
        {
            insert_msg.can_id = static_cast<unsigned int>(std::stoi(value));
        }
        else if ("can_len")
        {
            insert_msg.can_len = static_cast<unsigned int>(std::stoi(value));
        }
        else if (col_name.substr(0, 9) == "can_data")
        {
            int idx = col_name[9] - '0'; // "can_data0" → idx=0
            if (idx >= 0 && idx < 8)
            {
                insert_msg.can_data[idx] = static_cast<unsigned int>(std::stoi(value));
            }
        }
    }
    result_vec->push_back(insert_msg);
    return 0;
}

static int select_exec_fun(sqlite3 *db, const char *sql, int(*cb)(void *, int, char **, char **), void *usr_data)
{
    char *err_msg;
    int rc = sqlite3_exec(db, sql, cb, usr_data, &err_msg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
    return rc;
}

void select_between_id(void *usr_data, int id1, int id2, std::string table_name, sqlite3 *db)
{
    std::string select_str = "select * from " + table_name + \
    " where id >= " + "'" + std::to_string(id1) + "'" + "and id <= " + + "'" + std::to_string(id2) + "'"\
    " order by id";
    std::cout << select_str << std::endl;
    select_exec_fun(db, select_str.c_str(), select_msg_2_db_template, (void *)usr_data);
}

void select_at_timestamp(void *usr_data, std::string timestamp, std::string table_name, sqlite3 *db)
{
    std::string select_str = "select * from " + table_name + \
    " where timestamp " + "='"  + timestamp + "'"+ \
    " order by id";
    select_exec_fun(db, select_str.c_str(), select_msg_2_db_template, (void *)usr_data);
}

void select_between_timestamp(void *usr_data, std::string timestamp1, std::string timestamp2, std::string table_name, sqlite3 *db)
{
    std::string select_str = "select * from " + table_name + \
    " where timestamp between " + "'"  + timestamp1 + "'"+ " and " + "'"  + timestamp2 + "'" + \
    " order by id";
    select_exec_fun(db, select_str.c_str(), select_msg_2_db_template, (void *)usr_data);
}

void test_aaaa()
{
    sqlite3 *db = open_db("./aaaa.db");
    std::string table_name = "dcdc_frame";

    string timestamps1 = "2025-10-27 15:47:23.619018";
    string timestamps2 = "2025-10-27 15:47:23.668022";
    vector<linestr_2_dbinfo_t>insert_data{};
    select_between_timestamp((void *)&insert_data, timestamps1, timestamps2, table_name, db);
    std::cout << insert_data.size() << std::endl;
    insert_data.clear();
    select_at_timestamp((void *)&insert_data, timestamps2, table_name, db);
    std::cout << insert_data.size() << std::endl;
    linestr_2_dbinfo_t insert_msg = insert_data.at(0);

    int id1 = 0;
    int id2 = insert_msg.id + 100;

    if (100 >= insert_msg.id)
    {
        id1 = 1;
    }
    else
    {
        id1 = 100 - insert_msg.id;
    }
    insert_data.clear();
    select_between_id((void *)&insert_data, id1, id2, table_name, db);
    std::cout << insert_data.size() << std::endl;

    (void)sqlite3_close(db);
}

void handle_can_log(std::string &log_path)
{
    std::ifstream log(log_path.c_str());
    std::string line;
    if (!log.is_open())
    {
        std::cerr << "open db error" << std::endl;
        return;
    }

    sqlite3 *db = open_db("./aaaa.db");
    std::string table_name = "dcdc_frame";

    drop_table_in_db(table_name, db);

    string create_table_str = "CREATE TABLE IF NOT EXISTS " + table_name + "(id INTEGER PRIMARY KEY, timestamp TEXT, \
        can_id INTEGER, \
        can_len INTEGER,\
        can_data0 INTEGER, \
        can_data1 INTEGER, \
        can_data2 INTEGER, \
        can_data3 INTEGER, \
        can_data4 INTEGER, \
        can_data5 INTEGER, \
        can_data6 INTEGER, \
        can_data7 INTEGER)";
    exec_in_db(create_table_str, db);

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO dcdc_frame(timestamp, can_id, can_len, \
        can_data0, \
        can_data1, \
        can_data2, \
        can_data3, \
        can_data4, \
        can_data5, \
        can_data6, \
        can_data7) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    while (std::getline(log, line))
    {
        // std::cout << line << std::endl;
        linestr_2_dbinfo_t insert_msg;
        string_info_2_dcdc_insert_msg(line, insert_msg);
        // print_insert_frame(insert_msg);
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, insert_msg.timestamp.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, insert_msg.can_id);
        sqlite3_bind_int(stmt, 3, insert_msg.can_len);
        for (unsigned int i = 0U; i < insert_msg.can_len; i++)
        {
            sqlite3_bind_int(stmt, i + 4, insert_msg.can_data[i]);
            // std::cout << "data[" << i + 1 << "]: " << insert_msg.can_data[i] << std::endl;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cout << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        }
    }
    sqlite3_finalize(stmt);
}

/* g++ dcdc_database.cpp dcdc_database.h -g -o test && ./test */
int main()
{
    std::string log_path = "./bc.log";
    handle_can_log(log_path);
}