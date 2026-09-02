#include <mariadb/mysql.h>  //mariadb函式庫
#include <stdio.h>

#include "mariadb.h"

static MYSQL *conn;

static int connect_check()
{
    if (conn == NULL) {
        fprintf(stderr, "mysql_init failed\n");
        return 1;
    }
    return 0;
}

int connection_mariadb()
{
    if (conn != NULL) return 0;
    else conn = mysql_init(NULL);
    
    if (connect_check()) return 1;
    if (mysql_real_connect(conn , "localhost" , "meme5305" , 
        "123456", "MEME53_05" , 0 , NULL , 0
    ) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }
    return 0;
}

int disconnection_mariadb()
{
    if (connect_check()) return 1;    
    mysql_close(conn);
    printf("MariaDB is disconnected!\n");
    return 0;
}

int insert_dht11_mariadb(unsigned char temperature[], unsigned char humidity[])
{
    if (connect_check()) return 1;

    char temp_str[16];
    char hum_str[16];
    char sql[128];

    sprintf(temp_str, "%u.%u", temperature[0], temperature[1]);
    sprintf(hum_str, "%u.%u", humidity[0], humidity[1]);
    sprintf(sql,
        "INSERT INTO dht11_data "
        "(temperature, humidity) "
        "VALUES (%s, %s)",
        temp_str,
        hum_str);

    if (mysql_query(conn, sql) != 0) {
        fprintf(stderr, "INSERT failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    return 0;
}

int select_latest_dht11_data()
{
    const char *select_sql = "SELECT * FROM dht11_data ORDER BY created_at DESC LIMIT 1;";
    if (mysql_query(conn, select_sql) != 0) {
        fprintf(stderr, "SELECT failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }
    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row; //很可能會查到很多列 沒有資料了 → 回傳 NULL
    while ((row = mysql_fetch_row(result)) != NULL) {
        printf("id = %s\n", row[0]);
        printf("temperature = %s\n", row[1]);
        printf("humidity = %s\n", row[2]);
    }
    mysql_free_result(result); //釋放 SELECT 查詢結果佔用的記憶體。
    return 0;
}

int insert_bh1750_mariadb(unsigned char lux[])
{
    if (connect_check()) return 1;

    char sql[128];

    sprintf(sql,
        "INSERT INTO bh1750_data "
        "(lux) "
        "VALUES (%s)",
        lux);

    if (mysql_query(conn, sql) != 0) {
        fprintf(stderr, "INSERT failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    return 0;
}

int select_latest_bh1750_data()
{
    const char *select_sql = "SELECT * FROM bh1750_data ORDER BY created_at DESC LIMIT 1;";
    if (mysql_query(conn, select_sql) != 0) {
        fprintf(stderr, "SELECT failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }
    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row; //很可能會查到很多列 沒有資料了 → 回傳 NULL
    while ((row = mysql_fetch_row(result)) != NULL) {
        printf("id = %s\n", row[0]);
        printf("lux = %s\n", row[1]);
    }
    mysql_free_result(result); //釋放 SELECT 查詢結果佔用的記憶體。
    return 0;
}